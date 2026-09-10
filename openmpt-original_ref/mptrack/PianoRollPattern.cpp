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
	ROWINDEX row = 0;
	CHANNELINDEX channel = 0;

	bool operator<(const CellKey &other) const
	{
		return row != other.row ? row < other.row : channel < other.channel;
	}
};

struct CellChange
{
	ModCommand before;
	ModCommand after;
};

struct Placement
{
	ROWINDEX row = 0;
	CHANNELINDEX requestedChannel = 0;
	CHANNELINDEX sourceChannel = 0;
	ModCommand::NOTE pitch = NOTE_NONE;
	ModCommand::INSTR instrument = 0;
	std::optional<ModCommand::VOL> volume;
	std::optional<NoteRef> source;
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
			projection.notes.push_back(std::move(note));
		}
	}
	return projection;
}

PianoRollPattern::EditResult PianoRollPattern::Apply(const Operation &operation)
{
	auto &sndFile = m_document.GetSoundFile();
	const CHANNELINDEX originalChannels = sndFile.GetNumChannels();
	if(m_document.AIOccupied())
		return Failure(_T("AI retained occupancy makes Piano Roll edits read-only."), originalChannels);
	if(!sndFile.Patterns.IsValidPat(operation.pattern))
		return Failure(_T("The selected Pattern no longer exists."), originalChannels);

	const auto &specs = sndFile.GetModSpecifications();
	const ROWINDEX rows = sndFile.Patterns[operation.pattern].GetNumRows();
	if(operation.type == OperationType::Resize && !specs.hasNoteOff)
		return Failure(_T("This module format cannot represent an exact note-off."), originalChannels);

	std::map<CellKey, CellChange> changes;
	std::set<CellKey> sourceCells;
	std::vector<Placement> placements;
	std::vector<NoteRef> validSources;
	std::vector<NoteRef> resizedDestinations;

	const ModCommand emptyCell{};
	auto current = [&](ROWINDEX row, CHANNELINDEX channel) -> const ModCommand &
	{
		if(channel >= originalChannels)
			return emptyCell;
		return *sndFile.Patterns[operation.pattern].GetpModCommand(row, channel);
	};
	auto mutableCell = [&](ROWINDEX row, CHANNELINDEX channel) -> ModCommand &
	{
		const CellKey key{row, channel};
		auto found = changes.find(key);
		if(found == changes.end())
			found = changes.emplace(key, CellChange{current(row, channel), current(row, channel)}).first;
		return found->second.after;
	};
	auto validateSource = [&](const NoteRef &source, ModCommand *&cell) -> bool
	{
		if(source.pattern != operation.pattern || source.row >= rows || source.channel >= originalChannels)
			return false;
		cell = sndFile.Patterns[operation.pattern].GetpModCommand(source.row, source.channel);
		return IsPitched(cell->note);
	};

	if(operation.type == OperationType::Insert)
	{
		if(operation.row >= rows || operation.channel >= originalChannels || operation.pitch < specs.noteMin || operation.pitch > specs.noteMax)
			return Failure(_T("The note position or pitch is outside this Pattern or module format."), originalChannels);
		if(!IsExistingInstrumentOrSample(sndFile, operation.instrument))
			return Failure(_T("Choose an existing instrument or sample before drawing a note."), originalChannels);
		if(operation.volume && (*operation.volume > 64 || !specs.HasVolCommand(VOLCMD_VOLUME)))
			return Failure(_T("The selected volume cannot be represented by this module format."), originalChannels);
		placements.push_back({operation.row, operation.channel, operation.channel, static_cast<ModCommand::NOTE>(operation.pitch), operation.instrument, operation.volume, {}});
	} else if(operation.type == OperationType::Paste)
	{
		if(operation.clipboard.empty())
			return Failure(_T("The Piano Roll clipboard is empty."), originalChannels);
		for(const auto &clip : operation.clipboard)
		{
			const uint64 targetRow = static_cast<uint64>(operation.row) + clip.relativeRow;
			const int targetPitch = operation.pitch + clip.relativePitch;
			if(targetRow >= rows || targetPitch < specs.noteMin || targetPitch > specs.noteMax || !IsExistingInstrumentOrSample(sndFile, clip.instrument)
				|| (clip.volume && (*clip.volume > 64 || !specs.HasVolCommand(VOLCMD_VOLUME))))
				return Failure(_T("The clipboard cannot be expressed in this Pattern or module format."), originalChannels);
			placements.push_back({static_cast<ROWINDEX>(targetRow), static_cast<CHANNELINDEX>(operation.channel + clip.relativeChannel), operation.channel,
				static_cast<ModCommand::NOTE>(targetPitch), clip.instrument, clip.volume, {}});
		}
	} else
	{
		if(operation.notes.empty())
			return Failure(_T("No Piano Roll notes are selected."), originalChannels);
		for(const auto &source : operation.notes)
		{
			ModCommand *cell = nullptr;
			if(!validateSource(source, cell))
				return Failure(_T("A selected note changed outside the Piano Roll."), originalChannels);
			const CellKey key{source.row, source.channel};
			if(!sourceCells.insert(key).second)
				continue;
			validSources.push_back(source);
		}
		if(validSources.empty())
			return Failure(_T("No Piano Roll notes are selected."), originalChannels);
	}

	if(operation.type == OperationType::Delete)
	{
		for(const auto &source : validSources)
			ClearEditableFields(mutableCell(source.row, source.channel));
	} else if(operation.type == OperationType::Transpose)
	{
		for(const auto &source : validSources)
		{
			const int pitch = current(source.row, source.channel).note + operation.pitchDelta;
			if(pitch < specs.noteMin || pitch > specs.noteMax)
				return Failure(_T("The transposed note is outside this module format's note range."), originalChannels);
			mutableCell(source.row, source.channel).note = static_cast<ModCommand::NOTE>(pitch);
		}
	} else if(operation.type == OperationType::Resize)
	{
		for(const auto &source : validSources)
		{
			const auto projection = Read(operation.pattern);
			auto note = std::find_if(projection.notes.begin(), projection.notes.end(), [&](const Note &candidate) { return candidate.id == source; });
			if(note == projection.notes.end()) return Failure(_T("A selected note changed outside the Piano Roll."), originalChannels);
			if(operation.resizeFromLeft)
			{
				const int64 startValue = static_cast<int64>(source.row) + operation.rowDelta;
				if(startValue < 0 || startValue >= note->endRow)
					return Failure(_T("The left note edge must stay before its Tracker termination."), originalChannels);
				const ROWINDEX newStart = static_cast<ROWINDEX>(startValue);
				if(newStart == source.row) continue;
				const ModCommand &sourceCell = current(source.row, source.channel);
				const ModCommand &target = current(newStart, source.channel);
				if(target.note != NOTE_NONE || target.instr != 0 || target.volcmd == VOLCMD_VOLUME
					|| (sourceCell.volcmd == VOLCMD_VOLUME && target.volcmd != VOLCMD_NONE))
					return Failure(_T("The requested left note edge conflicts with Tracker note, instrument, or volume data."), originalChannels);
				ClearEditableFields(mutableCell(source.row, source.channel));
				ModCommand &destination = mutableCell(newStart, source.channel);
				destination.note = sourceCell.note;
				destination.instr = sourceCell.instr;
				if(sourceCell.volcmd == VOLCMD_VOLUME)
				{
					destination.volcmd = VOLCMD_VOLUME;
					destination.vol = sourceCell.vol;
				}
				resizedDestinations.push_back({operation.pattern, newStart, source.channel});
				continue;
			}

			const uint64 endValue = static_cast<uint64>(source.row) + std::max<ROWINDEX>(1, operation.length);
			if(endValue > rows)
				return Failure(_T("The right note edge is outside this Pattern."), originalChannels);
			const ROWINDEX newEnd = static_cast<ROWINDEX>(endValue);
			if(newEnd == note->endRow) continue;
			if(newEnd > note->endRow)
			{
				// An implicit end is another pitched or special Tracker event and
				// cannot be moved. An explicit key-off may instead be moved right
				// or removed to use the Pattern end as the implicit endpoint.
				if(note->endRow >= rows || current(note->endRow, source.channel).note != NOTE_KEYOFF)
					return Failure(_T("A note cannot be lengthened through the next Tracker note or termination event."), originalChannels);
				for(ROWINDEX row = static_cast<ROWINDEX>(note->endRow + 1); row < newEnd; ++row)
					if(current(row, source.channel).note != NOTE_NONE)
						return Failure(_T("A note cannot be lengthened through the next Tracker note or termination event."), originalChannels);
				mutableCell(note->endRow, source.channel).note = NOTE_NONE;
				if(newEnd < rows)
				{
					if(current(newEnd, source.channel).note != NOTE_NONE)
						return Failure(_T("The requested note-off conflicts with existing Tracker data."), originalChannels);
					mutableCell(newEnd, source.channel).note = NOTE_KEYOFF;
				}
				continue;
			}
			const ModCommand &target = current(newEnd, source.channel);
			if(target.note != NOTE_NONE)
				return Failure(_T("The requested note-off conflicts with existing Tracker data."), originalChannels);
			mutableCell(newEnd, source.channel).note = NOTE_KEYOFF;
			if(note->endRow < rows && current(note->endRow, source.channel).note == NOTE_KEYOFF)
				mutableCell(note->endRow, source.channel).note = NOTE_NONE;
		}
		validSources.insert(validSources.end(), resizedDestinations.begin(), resizedDestinations.end());
	} else if(operation.type == OperationType::Move)
	{
		for(const auto &source : validSources)
		{
			const int64 row = static_cast<int64>(source.row) + operation.rowDelta;
			const int pitch = current(source.row, source.channel).note + operation.pitchDelta;
			if(row < 0 || row >= rows || pitch < specs.noteMin || pitch > specs.noteMax)
				return Failure(_T("The moved note is outside this Pattern or module format."), originalChannels);
			const ModCommand &cell = current(source.row, source.channel);
			placements.push_back({static_cast<ROWINDEX>(row), source.channel, source.channel, static_cast<ModCommand::NOTE>(pitch), cell.instr,
				cell.volcmd == VOLCMD_VOLUME ? std::optional<ModCommand::VOL>(cell.vol) : std::nullopt, source});
		}
	}

	// Delete sources in the virtual result before resolving destinations.  This
	// makes a swap or a move into another selected cell deterministic, while all
	// non-editable fields in those cells stay byte-for-byte unchanged.
	if(operation.type == OperationType::Move)
		for(const auto &source : validSources)
			ClearEditableFields(mutableCell(source.row, source.channel));

	CHANNELINDEX plannedChannels = originalChannels;
	std::vector<CHANNELINDEX> newChannelSources;
	std::set<CellKey> placed;
	auto effective = [&](ROWINDEX row, CHANNELINDEX channel) -> const ModCommand &
	{
		const auto found = changes.find({row, channel});
		return found == changes.end() ? current(row, channel) : found->second.after;
	};
	auto canUse = [&](const Placement &placement, CHANNELINDEX channel) -> bool
	{
		const CellKey key{placement.row, channel};
		if(placed.count(key)) return false;
		const auto &target = effective(placement.row, channel);
		if(target.note != NOTE_NONE || target.instr != 0) return false;
		if(target.volcmd == VOLCMD_VOLUME) return false;
		if(placement.volume && target.volcmd != VOLCMD_NONE) return false;
		return true;
	};
	auto resolveChannel = [&](const Placement &placement, CHANNELINDEX &resolved) -> bool
	{
		for(CHANNELINDEX offset = 0; offset < originalChannels; ++offset)
		{
			const CHANNELINDEX candidate = static_cast<CHANNELINDEX>((placement.requestedChannel + offset) % originalChannels);
			if(operation.viewFilter.IsVisible(candidate, originalChannels) && canUse(placement, candidate))
			{
				resolved = candidate;
				return true;
			}
		}
		for(CHANNELINDEX channel = originalChannels; channel < plannedChannels; ++channel)
		{
			if(canUse(placement, channel))
			{
				resolved = channel;
				return true;
			}
		}
		if(plannedChannels >= specs.channelsMax) return false;
		resolved = plannedChannels++;
		newChannelSources.push_back(std::min<CHANNELINDEX>(placement.sourceChannel, originalChannels - 1));
		return true;
	};

	for(const auto &placement : placements)
	{
		// PC / PCS and the Tracker's other special note events do not describe
		// a graphical note. They are not ordinary occupancy that may silently
		// be routed around: accepting a request whose intended cell contains one
		// would make a multi-note paste non-deterministic and could disguise an
		// unrepresentable edit. Effects and non-volume volume commands remain
		// valid pure-effect destinations and are deliberately not included here.
		if(placement.requestedChannel < originalChannels
			&& current(placement.row, placement.requestedChannel).note >= NOTE_MIN_SPECIAL)
			return Failure(_T("A Piano Roll edit cannot target a PC, PCS, or other special-note cell."), originalChannels);
		CHANNELINDEX channel = 0;
		if(!resolveChannel(placement, channel))
			return Failure(_T("No visible free channel is available and the module has reached its channel limit."), originalChannels);
		ModCommand &destination = mutableCell(placement.row, channel);
		destination.note = placement.pitch;
		destination.instr = placement.instrument;
		if(placement.volume)
		{
			destination.volcmd = VOLCMD_VOLUME;
			destination.vol = *placement.volume;
		}
		placed.insert({placement.row, channel});
		validSources.push_back({operation.pattern, placement.row, channel});
	}

	bool changed = false;
	for(const auto &[key, change] : changes)
	{
		if(change.before.note != change.after.note || change.before.instr != change.after.instr
			|| change.before.volcmd != change.after.volcmd || change.before.vol != change.after.vol)
		{
			changed = true;
			break;
		}
	}
	if(!changed)
		return Failure(_T("The requested Piano Roll edit makes no change."), originalChannels);

	const CString undoText = operation.undoName.IsEmpty() ? DefaultUndoName(operation.type) : operation.undoName;
	const std::string undoName = mpt::ToCharset(mpt::Charset::Locale, undoText);
	const bool addingChannels = plannedChannels != originalChannels;
	if(addingChannels)
	{
		if(!PrepareAllPatternUndo(m_document, undoName.c_str()))
			return Failure(_T("OpenMPT could not prepare an Undo step; no data was changed."), originalChannels);
		std::vector<CHANNELINDEX> channelOrder(plannedChannels, CHANNELINDEX_INVALID);
		std::iota(channelOrder.begin(), channelOrder.begin() + originalChannels, CHANNELINDEX(0));
		for(CHANNELINDEX index = originalChannels; index < plannedChannels; ++index)
			channelOrder[index] = newChannelSources[index - originalChannels];
		if(m_document.ReArrangeChannels(channelOrder, false) != plannedChannels)
		{
			RemoveAllPatternUndo(m_document);
			return Failure(_T("OpenMPT could not allocate the required channels; no data was changed."), originalChannels);
		}
	} else if(!m_document.GetPatternUndo().PrepareUndo(operation.pattern, 0, 0, originalChannels, rows, undoName.c_str()))
	{
		return Failure(_T("OpenMPT could not prepare an Undo step; no data was changed."), originalChannels);
	}

	{
		CriticalSection guard;
		for(const auto &[key, change] : changes)
		{
			ModCommand &destination = *sndFile.Patterns[operation.pattern].GetpModCommand(key.row, key.channel);
			// Deliberately never assign a whole ModCommand: effect fields, PC/PCS,
			// and non-volume volume commands must survive every Piano Roll edit.
			destination.note = change.after.note;
			destination.instr = change.after.instr;
			destination.volcmd = change.after.volcmd;
			destination.vol = change.after.vol;
		}
	}
	m_document.SetModified();
	m_document.UpdateAllViews(nullptr, PatternHint(operation.pattern).Data().Undo());
	if(addingChannels)
		m_document.UpdateAllViews(nullptr, GeneralHint().Channels().ModType());

	EditResult result;
	result.applied = true;
	result.channelsAdded = addingChannels;
	result.channels = plannedChannels;
	result.affected = std::move(validSources);
	return result;
}

OPENMPT_NAMESPACE_END
