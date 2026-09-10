/*
 * PianoRollPattern.cpp
 * --------------------
 * Implementation of the Piano Roll projection and its atomic Tracker edits.
 */

#include "stdafx.h"
#include "PianoRollPattern.h"

#include "Moddoc.h"
#include "Undo.h"
#include "UpdateHints.h"
#include "../soundlib/AudioCriticalSection.h"
#include "../soundlib/mod_specifications.h"

#include <map>
#include <set>

OPENMPT_NAMESPACE_BEGIN

namespace
{
using Operation = PianoRollPattern::Operation;
using NoteRef = PianoRollPattern::NoteRef;
using EditResult = PianoRollPattern::EditResult;
using ViewFilter = PianoRollPattern::ViewFilter;

struct CellKey
{
	PATTERNINDEX pattern = 0;
	ROWINDEX row = 0;
	CHANNELINDEX channel = 0;

	bool operator<(const CellKey &other) const
	{
		if(pattern != other.pattern) return pattern < other.pattern;
		return row != other.row ? row < other.row : channel < other.channel;
	}
};

struct CellChange
{
	ModCommand before;
	ModCommand after;
};

struct NoteBlock
{
	PATTERNINDEX pattern = 0;
	ROWINDEX start = 0, end = 1;
	CHANNELINDEX sourceChannel = 0;
	ModCommand::NOTE pitch = NOTE_NONE;
	ModCommand::INSTR instrument = 0;
	std::optional<ModCommand::VOL> volume;
	bool affected = false;
};

bool IsTermination(const ModCommand &cell)
{
	return ModCommand::IsNote(cell.note) || cell.note == NOTE_KEYOFF || cell.note == NOTE_NOTECUT || cell.note == NOTE_FADE
		|| cell.command == CMD_KEYOFF || cell.command == CMD_DELAYCUT;
}

bool IsPitched(const ModCommand::NOTE note)
{
	return ModCommand::IsNote(note);
}

void ClearEditableFields(ModCommand &cell)
{
	cell.note = NOTE_NONE;
	cell.instr = 0;
	if(cell.volcmd == VOLCMD_VOLUME)
	{
		cell.volcmd = VOLCMD_NONE;
		cell.vol = 0;
	}
}

bool IsExistingInstrumentOrSample(const CSoundFile &sndFile, ModCommand::INSTR instrument)
{
	if(instrument == 0)
		return false;
	if(sndFile.GetNumInstruments() > 0)
		return instrument <= sndFile.GetNumInstruments() && sndFile.Instruments[instrument] != nullptr;
	return instrument <= sndFile.GetNumSamples()
		&& (sndFile.GetSample(instrument).HasSampleData() || sndFile.GetSample(instrument).uFlags[CHN_ADLIB]);
}

ROWINDEX RowsPerBeat(const CSoundFile &sndFile, PATTERNINDEX pattern)
{
	const auto &pat = sndFile.Patterns[pattern];
	return pat.GetOverrideSignature() ? pat.GetRowsPerBeat() : sndFile.m_nDefaultRowsPerBeat;
}

ROWINDEX RowsPerMeasure(const CSoundFile &sndFile, PATTERNINDEX pattern)
{
	const auto &pat = sndFile.Patterns[pattern];
	return pat.GetOverrideSignature() ? pat.GetRowsPerMeasure() : sndFile.m_nDefaultRowsPerMeasure;
}

CString DefaultUndoName(const PianoRollPattern::OperationType type)
{
	switch(type)
	{
	case PianoRollPattern::OperationType::Insert: return _T("Piano Roll: Draw Note");
	case PianoRollPattern::OperationType::Delete: return _T("Piano Roll: Delete Notes");
	case PianoRollPattern::OperationType::Move: return _T("Piano Roll: Move Notes");
	case PianoRollPattern::OperationType::Transpose: return _T("Piano Roll: Transpose Notes");
	case PianoRollPattern::OperationType::Resize: return _T("Piano Roll: Resize Notes");
	case PianoRollPattern::OperationType::Paste: return _T("Piano Roll: Paste Notes");
	case PianoRollPattern::OperationType::NormalizeChannels: return _T("Piano Roll: Split Channels by Instrument");
	}
	return _T("Piano Roll Edit");
}

EditResult Failure(const CString &reason, CHANNELINDEX channels)
{
	EditResult result;
	result.reason = reason;
	result.channels = channels;
	return result;
}

// CPatternUndo represents a multi-pattern edit with linked entries.  This
// helper makes preparation transactional: if one allocation fails, no partial
// step is left on the native undo stack.
bool PrepareAllPatternUndo(CModDoc &document, const char *description)
{
	auto &sndFile = document.GetSoundFile();
	std::vector<PATTERNINDEX> patterns;
	for(PATTERNINDEX pattern = 0; pattern < sndFile.Patterns.Size(); ++pattern)
		if(sndFile.Patterns.IsValidPat(pattern)) patterns.push_back(pattern);

	bool linked = false;
	size_t prepared = 0;
	for(PATTERNINDEX pattern : patterns)
	{
		if(!document.GetPatternUndo().PrepareUndo(pattern, 0, 0, sndFile.GetNumChannels(), sndFile.Patterns[pattern].GetNumRows(), description, linked, pattern == patterns.back()))
		{
			while(prepared-- > 0)
				document.GetPatternUndo().RemoveLastUndoStep();
			return false;
		}
		linked = true;
		prepared++;
	}
	return !patterns.empty();
}

void RemoveAllPatternUndo(CModDoc &document)
{
	const auto &sndFile = document.GetSoundFile();
	size_t count = 0;
	for(PATTERNINDEX pattern = 0; pattern < sndFile.Patterns.Size(); ++pattern)
		if(sndFile.Patterns.IsValidPat(pattern)) count++;
	while(count-- > 0)
		document.GetPatternUndo().RemoveLastUndoStep();
}

}  // namespace

bool PianoRollPattern::ViewFilter::IsVisible(CHANNELINDEX channel, CHANNELINDEX channels) const
{
	return channel < channels && (visibleChannels.empty() || (channel < visibleChannels.size() && visibleChannels[channel]));
}

PianoRollPattern::Projection PianoRollPattern::Read(PATTERNINDEX pattern, const ViewFilter &viewFilter) const
{
	Projection projection;
	projection.pattern = pattern;
	const auto &sndFile = m_document.GetSoundFile();
	if(!sndFile.Patterns.IsValidPat(pattern))
	{
		projection.nonEditableReason = _T("The selected Pattern no longer exists.");
		return projection;
	}

	const auto &specs = sndFile.GetModSpecifications();
	const auto &source = sndFile.Patterns[pattern];
	projection.rows = source.GetNumRows();
	projection.channels = sndFile.GetNumChannels();
	projection.rowsPerBeat = RowsPerBeat(sndFile, pattern);
	projection.rowsPerMeasure = RowsPerMeasure(sndFile, pattern);
	projection.noteMin = specs.noteMin;
	projection.noteMax = specs.noteMax;
	projection.editable = !m_document.AIOccupied();
	projection.canResizeLength = specs.hasNoteOff;
	if(!projection.editable)
		projection.nonEditableReason = _T("AI retained occupancy makes Piano Roll edits read-only.");
	if(!projection.canResizeLength)
		projection.resizeDisabledReason = _T("This module format has no note-off event, so note lengths cannot be written exactly.");

	for(CHANNELINDEX channel = 0; channel < projection.channels; ++channel)
	{
		if(!viewFilter.IsVisible(channel, projection.channels))
			continue;
		for(ROWINDEX row = 0; row < projection.rows; ++row)
		{
			const ModCommand &cell = *source.GetpModCommand(row, channel);
			if(!IsPitched(cell.note))
				continue;
			ROWINDEX end = projection.rows;
			for(ROWINDEX next = row + 1; next < projection.rows; ++next)
			{
				if(IsTermination(*source.GetpModCommand(next, channel)))
				{
					end = next;
					break;
				}
			}
			Note note;
			note.id = {pattern, row, channel};
			note.pitch = cell.note;
			note.endRow = end;
			note.instrument = cell.instr;
			if(cell.volcmd == VOLCMD_VOLUME)
				note.volume = cell.vol;
			note.volumeCommand = cell.volcmd;
			note.volumeParameter = cell.vol;
			note.effectCommand = cell.command;
			note.effectParameter = cell.param;
			projection.notes.push_back(std::move(note));
		}
	}
	return projection;
}

namespace
{

EditResult ApplyRepacked(CModDoc &document, const Operation &operation)
{
	auto &sndFile = document.GetSoundFile();
	const CHANNELINDEX originalChannels = sndFile.GetNumChannels();
	if(document.AIOccupied()) return Failure(_T("AI retained occupancy makes Piano Roll edits read-only."), originalChannels);
	if(!sndFile.Patterns.IsValidPat(operation.pattern)) return Failure(_T("The selected Pattern no longer exists."), originalChannels);
	const auto &specs = sndFile.GetModSpecifications();
	if(!specs.hasNoteOff) return Failure(_T("This module format cannot represent explicit Piano Roll note lengths."), originalChannels);

	// Resolve instrument memory at the channel level before turning Tracker
	// events into explicit Piano Roll blocks. A mixed channel is precisely what
	// this normalization is designed to split.
	std::vector<std::set<ModCommand::INSTR>> channelInstruments(originalChannels);
	for(PATTERNINDEX pat = 0; pat < sndFile.Patterns.Size(); ++pat)
	{
		if(!sndFile.Patterns.IsValidPat(pat)) continue;
		for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
			for(ROWINDEX row = 0; row < sndFile.Patterns[pat].GetNumRows(); ++row)
			{
				const auto &cell = *sndFile.Patterns[pat].GetpModCommand(row, channel);
				if(ModCommand::IsNote(cell.note) && cell.instr != 0) channelInstruments[channel].insert(cell.instr);
			}
	}

	std::vector<NoteBlock> blocks;
	std::set<CellKey> managedStops;
	for(PATTERNINDEX pat = 0; pat < sndFile.Patterns.Size(); ++pat)
	{
		if(!sndFile.Patterns.IsValidPat(pat)) continue;
		const auto &pattern = sndFile.Patterns[pat];
		for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
		{
			ModCommand::INSTR remembered = channelInstruments[channel].size() == 1 ? *channelInstruments[channel].begin() : 0;
			for(ROWINDEX row = 0; row < pattern.GetNumRows(); ++row)
			{
				const ModCommand &cell = *pattern.GetpModCommand(row, channel);
				if(cell.instr != 0 && ModCommand::IsNote(cell.note)) remembered = cell.instr;
				if(!ModCommand::IsNote(cell.note)) continue;
				ROWINDEX end = pattern.GetNumRows();
				for(ROWINDEX next = row + 1; next < pattern.GetNumRows(); ++next)
				{
					const ModCommand &termination = *pattern.GetpModCommand(next, channel);
					if(!IsTermination(termination)) continue;
					end = next;
					if(termination.note == NOTE_KEYOFF) managedStops.insert({pat, next, channel});
					break;
				}
				blocks.push_back({pat, row, end, channel, cell.note, cell.instr ? cell.instr : remembered,
					cell.volcmd == VOLCMD_VOLUME ? std::optional<ModCommand::VOL>(cell.vol) : std::nullopt, false});
			}
		}
	}

	auto findBlock = [&](const NoteRef &ref) -> NoteBlock *
	{
		auto found = std::find_if(blocks.begin(), blocks.end(), [&](const NoteBlock &block)
		{
			return block.pattern == ref.pattern && block.start == ref.row && block.sourceChannel == ref.channel;
		});
		return found == blocks.end() ? nullptr : &*found;
	};

	if(operation.type == PianoRollPattern::OperationType::Insert)
	{
		const ROWINDEX rows = sndFile.Patterns[operation.pattern].GetNumRows();
		if(operation.row >= rows || operation.pitch < specs.noteMin || operation.pitch > specs.noteMax
			|| !IsExistingInstrumentOrSample(sndFile, operation.instrument)
			|| (operation.volume && (*operation.volume > 64 || !specs.HasVolCommand(VOLCMD_VOLUME))))
			return Failure(_T("Choose an existing instrument and a valid empty Piano Roll position."), originalChannels);
		const ROWINDEX end = static_cast<ROWINDEX>(std::min<uint64>(rows, static_cast<uint64>(operation.row) + std::max<ROWINDEX>(1, operation.length)));
		blocks.push_back({operation.pattern, operation.row, end, operation.channel, static_cast<ModCommand::NOTE>(operation.pitch), operation.instrument, operation.volume, true});
	} else if(operation.type == PianoRollPattern::OperationType::Paste)
	{
		if(operation.clipboard.empty()) return Failure(_T("The Piano Roll clipboard is empty."), originalChannels);
		const ROWINDEX rows = sndFile.Patterns[operation.pattern].GetNumRows();
		for(const auto &clip : operation.clipboard)
		{
			const uint64 start = static_cast<uint64>(operation.row) + clip.relativeRow;
			const uint64 end = start + std::max<ROWINDEX>(1, clip.length);
			const int pitch = operation.pitch + clip.relativePitch;
			if(start >= rows || end > rows || pitch < specs.noteMin || pitch > specs.noteMax || !IsExistingInstrumentOrSample(sndFile, clip.instrument)
				|| (clip.volume && (*clip.volume > 64 || !specs.HasVolCommand(VOLCMD_VOLUME))))
				return Failure(_T("The clipboard cannot be expressed in this Pattern or module format."), originalChannels);
			blocks.push_back({operation.pattern, static_cast<ROWINDEX>(start), static_cast<ROWINDEX>(end), operation.channel,
				static_cast<ModCommand::NOTE>(pitch), clip.instrument, clip.volume, true});
		}
	} else if(operation.type != PianoRollPattern::OperationType::NormalizeChannels)
	{
		if(operation.notes.empty()) return Failure(_T("No Piano Roll notes are selected."), originalChannels);
		std::vector<NoteBlock *> selected;
		for(const auto &ref : operation.notes)
		{
			NoteBlock *block = findBlock(ref);
			if(!block) return Failure(_T("A selected note changed outside the Piano Roll."), originalChannels);
			if(std::find(selected.begin(), selected.end(), block) == selected.end()) selected.push_back(block);
		}
		if(operation.type == PianoRollPattern::OperationType::Delete)
		{
			std::set<std::tuple<PATTERNINDEX, ROWINDEX, CHANNELINDEX>> deleting;
			for(const NoteBlock *block : selected) deleting.emplace(block->pattern, block->start, block->sourceChannel);
			blocks.erase(std::remove_if(blocks.begin(), blocks.end(), [&](const NoteBlock &block)
			{
				return deleting.count({block.pattern, block.start, block.sourceChannel}) != 0;
			}), blocks.end());
		} else
		{
			for(NoteBlock *block : selected)
			{
				block->affected = true;
				if(operation.type == PianoRollPattern::OperationType::Transpose)
				{
					const int pitch = block->pitch + operation.pitchDelta;
					if(pitch < specs.noteMin || pitch > specs.noteMax) return Failure(_T("The transposed note is outside this module format's note range."), originalChannels);
					block->pitch = static_cast<ModCommand::NOTE>(pitch);
				} else if(operation.type == PianoRollPattern::OperationType::Move)
				{
					const int64 start = static_cast<int64>(block->start) + operation.rowDelta;
					const int64 end = static_cast<int64>(block->end) + operation.rowDelta;
					const int pitch = block->pitch + operation.pitchDelta;
					if(start < 0 || end > sndFile.Patterns[block->pattern].GetNumRows() || pitch < specs.noteMin || pitch > specs.noteMax)
						return Failure(_T("The moved note is outside this Pattern or module format."), originalChannels);
					block->start = static_cast<ROWINDEX>(start);
					block->end = static_cast<ROWINDEX>(end);
					block->pitch = static_cast<ModCommand::NOTE>(pitch);
				} else if(operation.type == PianoRollPattern::OperationType::Resize)
				{
					if(operation.resizeFromLeft)
					{
						const int64 start = static_cast<int64>(block->start) + operation.rowDelta;
						if(start < 0 || start >= block->end) return Failure(_T("The left note edge must stay before its end."), originalChannels);
						block->start = static_cast<ROWINDEX>(start);
					} else
					{
						const uint64 end = static_cast<uint64>(block->start) + std::max<ROWINDEX>(1, operation.length);
						if(end > sndFile.Patterns[block->pattern].GetNumRows()) return Failure(_T("The right note edge is outside this Pattern."), originalChannels);
						block->end = static_cast<ROWINDEX>(end);
					}
				}
			}
		}
	}

	// Stable interval partitioning is the "falling bricks" rule: each
	// instrument owns a channel group, and each note takes the lowest layer that
	// is free for its whole explicit half-open interval.
	std::map<ModCommand::INSTR, std::vector<NoteBlock *>> groups;
	for(auto &block : blocks) groups[block.instrument].push_back(&block);
	std::map<ModCommand::INSTR, size_t> layerCounts;
	std::map<NoteBlock *, size_t> layers;
	for(auto &[instrument, notes] : groups)
	{
		std::map<PATTERNINDEX, std::vector<ROWINDEX>> layerEnds;
		std::stable_sort(notes.begin(), notes.end(), [](const NoteBlock *a, const NoteBlock *b)
		{
			if(a->pattern != b->pattern) return a->pattern < b->pattern;
			if(a->start != b->start) return a->start < b->start;
			return a->sourceChannel < b->sourceChannel;
		});
		for(NoteBlock *block : notes)
		{
			auto &ends = layerEnds[block->pattern];
			size_t layer = 0;
			while(layer < ends.size() && ends[layer] > block->start) layer++;
			if(layer == ends.size()) ends.push_back(0);
			ends[layer] = block->end;
			layers[block] = layer;
			layerCounts[instrument] = std::max(layerCounts[instrument], layer + 1);
		}
	}

	size_t requiredChannels = 0;
	for(const auto &[instrument, count] : layerCounts) requiredChannels += count;
	const CHANNELINDEX workingChannels = static_cast<CHANNELINDEX>(std::max<size_t>(requiredChannels, originalChannels));
	if(workingChannels > specs.channelsMax) return Failure(_T("The instrument groups need more channels than this module format permits."), originalChannels);
	std::map<ModCommand::INSTR, CHANNELINDEX> groupStarts;
	CHANNELINDEX nextChannel = 0;
	for(const auto &[instrument, count] : layerCounts)
	{
		groupStarts[instrument] = nextChannel;
		nextChannel = static_cast<CHANNELINDEX>(nextChannel + count);
	}

	std::map<CellKey, CellChange> changes;
	auto originalCell = [&](PATTERNINDEX pat, ROWINDEX row, CHANNELINDEX channel) -> ModCommand
	{
		if(channel >= originalChannels) return {};
		return *sndFile.Patterns[pat].GetpModCommand(row, channel);
	};
	auto mutableCell = [&](PATTERNINDEX pat, ROWINDEX row, CHANNELINDEX channel) -> ModCommand &
	{
		CellKey key{pat, row, channel};
		auto found = changes.find(key);
		if(found == changes.end())
		{
			const ModCommand cell = originalCell(pat, row, channel);
			found = changes.emplace(key, CellChange{cell, cell}).first;
		}
		return found->second.after;
	};
	for(PATTERNINDEX pat = 0; pat < sndFile.Patterns.Size(); ++pat)
	{
		if(!sndFile.Patterns.IsValidPat(pat)) continue;
		for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
			for(ROWINDEX row = 0; row < sndFile.Patterns[pat].GetNumRows(); ++row)
			{
				const ModCommand cell = originalCell(pat, row, channel);
				if(ModCommand::IsNote(cell.note)) ClearEditableFields(mutableCell(pat, row, channel));
				else if(managedStops.count({pat, row, channel})) mutableCell(pat, row, channel).note = NOTE_NONE;
			}
	}
	std::set<CellKey> starts;
	std::vector<NoteRef> affected;
	for(NoteBlock &block : blocks)
	{
		const CHANNELINDEX channel = static_cast<CHANNELINDEX>(groupStarts[block.instrument] + layers[&block]);
		ModCommand &cell = mutableCell(block.pattern, block.start, channel);
		if(cell.note >= NOTE_MIN_SPECIAL || cell.instr != 0 || cell.volcmd == VOLCMD_VOLUME)
			return Failure(_T("A Piano Roll note conflicts with Tracker-only data while splitting channels."), originalChannels);
		cell.note = block.pitch;
		cell.instr = block.instrument;
		if(block.volume)
		{
			cell.volcmd = VOLCMD_VOLUME;
			cell.vol = *block.volume;
		}
		starts.insert({block.pattern, block.start, channel});
		if(block.affected) affected.push_back({block.pattern, block.start, channel});
	}
	for(NoteBlock &block : blocks)
	{
		if(block.end >= sndFile.Patterns[block.pattern].GetNumRows()) continue;
		const CHANNELINDEX channel = static_cast<CHANNELINDEX>(groupStarts[block.instrument] + layers[&block]);
		if(starts.count({block.pattern, block.end, channel})) continue;
		ModCommand &cell = mutableCell(block.pattern, block.end, channel);
		if(cell.note == NOTE_NONE) cell.note = NOTE_KEYOFF;
	}

	// Keep channels that still contain Tracker-only data, but discard empty
	// trailing layers created by earlier Piano Roll overlap operations.
	size_t usedChannels = requiredChannels;
	for(CHANNELINDEX channel = 0; channel < workingChannels; ++channel)
	{
		bool used = false;
		for(PATTERNINDEX pat = 0; pat < sndFile.Patterns.Size() && !used; ++pat)
		{
			if(!sndFile.Patterns.IsValidPat(pat)) continue;
			for(ROWINDEX row = 0; row < sndFile.Patterns[pat].GetNumRows(); ++row)
			{
				const CellKey key{pat, row, channel};
				const auto changedCell = changes.find(key);
				const ModCommand &cell = changedCell != changes.end() ? changedCell->second.after : originalCell(pat, row, channel);
				if(!cell.IsEmpty()) { used = true; break; }
			}
		}
		if(used) usedChannels = std::max(usedChannels, static_cast<size_t>(channel + 1));
	}
	const CHANNELINDEX plannedChannels = static_cast<CHANNELINDEX>(std::max<size_t>(usedChannels, specs.channelsMin));

	bool changed = plannedChannels != originalChannels;
	for(const auto &[key, change] : changes) if(change.before != change.after) { changed = true; break; }
	if(!changed) return Failure(_T("The requested Piano Roll edit makes no change."), originalChannels);
	const CString undoText = operation.undoName.IsEmpty() ? DefaultUndoName(operation.type) : operation.undoName;
	const std::string undoName = mpt::ToCharset(mpt::Charset::Locale, undoText);
	if(!PrepareAllPatternUndo(document, undoName.c_str())) return Failure(_T("OpenMPT could not prepare an Undo step; no data was changed."), originalChannels);
	if(plannedChannels != originalChannels)
	{
		std::vector<CHANNELINDEX> order(plannedChannels, CHANNELINDEX_INVALID);
		std::iota(order.begin(), order.begin() + std::min(originalChannels, plannedChannels), CHANNELINDEX(0));
		if(document.ReArrangeChannels(order, false) != plannedChannels)
		{
			RemoveAllPatternUndo(document);
			return Failure(_T("OpenMPT could not allocate the required instrument layers; no data was changed."), originalChannels);
		}
	}
	{
		CriticalSection guard;
		for(const auto &[key, change] : changes)
			if(key.channel < plannedChannels)
				*sndFile.Patterns[key.pattern].GetpModCommand(key.row, key.channel) = change.after;
	}
	document.SetModified();
	document.UpdateAllViews(nullptr, GeneralHint().Channels().ModType());
	EditResult result;
	result.applied = true;
	result.channelsAdded = plannedChannels != originalChannels;
	result.channels = plannedChannels;
	result.affected = std::move(affected);
	return result;
}

} // namespace

PianoRollPattern::EditResult PianoRollPattern::Apply(const Operation &operation)
{
	return ApplyRepacked(m_document, operation);
}

OPENMPT_NAMESPACE_END
