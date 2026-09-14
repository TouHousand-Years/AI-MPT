/*
 * mpt_tests_pianoroll.cpp
 * -----------------------
 * Projection and transactional edit coverage for the standalone Piano Roll.
 */

#include "stdafx.h"
#ifdef ENABLE_TESTS

#include "../mptrack/AIPattern.h"
#include "../mptrack/Moddoc.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/PianoRollPattern.h"

#include <stdexcept>

OPENMPT_NAMESPACE_BEGIN

namespace Test
{

namespace
{

void Require(bool condition, const char *message)
{
	if(!condition)
		throw std::runtime_error(message);
}

bool IsEmpty(const ModCommand &cell)
{
	return cell.note == NOTE_NONE && cell.instr == 0 && cell.volcmd == VOLCMD_NONE && cell.vol == 0
		&& cell.command == CMD_NONE && cell.param == 0;
}

struct EmptySpan
{
	ROWINDEX row;
	CHANNELINDEX channel;
};

EmptySpan FindEmptySpan(const CSoundFile &sndFile, PATTERNINDEX pattern, ROWINDEX length)
{
	const CPattern &source = sndFile.Patterns[pattern];
	for(CHANNELINDEX channel = 0; channel < sndFile.GetNumChannels(); ++channel)
	{
		for(ROWINDEX row = 0; row + length <= source.GetNumRows(); ++row)
		{
			bool empty = true;
			for(ROWINDEX offset = 0; offset < length; ++offset)
				empty = empty && IsEmpty(*source.GetpModCommand(row + offset, channel));
			if(empty)
				return {row, channel};
		}
	}
	throw std::runtime_error("The Piano Roll fixture has no usable empty span");
}

void RequireInstrumentChannels(const CSoundFile &sndFile)
{
	for(CHANNELINDEX channel = 0; channel < sndFile.GetNumChannels(); ++channel)
	{
		std::set<ModCommand::INSTR> instruments;
		for(PATTERNINDEX pattern = 0; pattern < sndFile.Patterns.Size(); ++pattern)
		{
			if(!sndFile.Patterns.IsValidPat(pattern)) continue;
			for(ROWINDEX row = 0; row < sndFile.Patterns[pattern].GetNumRows(); ++row)
			{
				const auto &cell = *sndFile.Patterns[pattern].GetpModCommand(row, channel);
				if(ModCommand::IsNote(cell.note) && cell.instr != 0) instruments.insert(cell.instr);
			}
		}
		Require(instruments.size() <= 1, "Every Piano Roll channel belongs to at most one instrument group");
	}
}

PianoRollPattern::Operation InsertAt(PATTERNINDEX pattern, ROWINDEX row, CHANNELINDEX channel, int pitch)
{
	PianoRollPattern::Operation operation;
	operation.type = PianoRollPattern::OperationType::Insert;
	operation.pattern = pattern;
	operation.row = row;
	operation.channel = channel;
	operation.pitch = pitch;
	operation.instrument = 1;
	return operation;
}

void UndoOne(CModDoc &document, const char *message)
{
	Require(document.GetPatternUndo().Undo() != PATTERNINDEX_INVALID, message);
}

}  // namespace


void PianoRollPatternTests(const CString &fixture)
{
	auto *document = static_cast<CModDoc *>(theApp.OpenDocumentFile(fixture, FALSE));
	if(document == nullptr)
		throw std::runtime_error("Cannot load Piano Roll fixture");

	auto &sndFile = document->GetSoundFile();
	constexpr PATTERNINDEX patternIndex = 0;
	PianoRollPattern editor(*document);
	const auto initial = editor.Read(patternIndex);
	Require(initial.rows == 128 && initial.channels == 4, "Projection preserves Pattern dimensions");
	Require(initial.rowsPerBeat == 4 && initial.rowsPerMeasure == 16, "Projection reads beat and measure timing");
	Require(initial.editable && initial.canResizeLength && !initial.notes.empty(), "MPTM projection is editable and note-off capable");
	for(const auto &note : initial.notes)
		Require(note.id.pattern == patternIndex && note.endRow > note.id.row && note.endRow <= initial.rows,
			"Every projected note has a bounded exclusive endpoint");

	const auto empty = FindEmptySpan(sndFile, patternIndex, 8);
	auto &first = *sndFile.Patterns[patternIndex].GetpModCommand(empty.row, empty.channel);
	auto &explicitOff = *sndFile.Patterns[patternIndex].GetpModCommand(empty.row + 3, empty.channel);
	const ModCommand savedFirst = first, savedOff = explicitOff;
	first.note = static_cast<ModCommand::NOTE>(NOTE_MIDDLEC);
	first.instr = 1;
	explicitOff.note = NOTE_KEYOFF;
	const auto explicitProjection = editor.Read(patternIndex);
	const auto explicitNote = std::find_if(explicitProjection.notes.begin(), explicitProjection.notes.end(), [&](const auto &note)
	{
		return note.id == PianoRollPattern::NoteRef{patternIndex, empty.row, empty.channel};
	});
	Require(explicitNote != explicitProjection.notes.end() && explicitNote->endRow == empty.row + 3,
		"Projection derives an explicit note-off endpoint");
	first = savedFirst;
	explicitOff = savedOff;

	// All editing crosses the PianoRollPattern seam. Normalization makes note
	// durations explicit and packs each instrument into the minimum non-
	// overlapping layers, independently of the Tracker channel cursor.
	PianoRollPattern::Operation normalize;
	normalize.type = PianoRollPattern::OperationType::NormalizeChannels;
	normalize.pattern = patternIndex;
	editor.Apply(normalize);
	RequireInstrumentChannels(sndFile);

	constexpr ROWINDEX brickRow = 112;
	for(CHANNELINDEX channel = 0; channel < sndFile.GetNumChannels(); ++channel)
	{
		for(ROWINDEX row = brickRow - 1; row < sndFile.Patterns[patternIndex].GetNumRows(); ++row)
			sndFile.Patterns[patternIndex].GetpModCommand(row, channel)->Clear();
		sndFile.Patterns[patternIndex].GetpModCommand(brickRow - 1, channel)->note = NOTE_KEYOFF;
	}
	auto insertBrick = InsertAt(patternIndex, brickRow, 0, NOTE_MIDDLEC + 7);
	insertBrick.length = 2;
	const auto firstBrick = editor.Apply(insertBrick);
	Require(firstBrick.applied && firstBrick.affected.size() == 1, "Double-click insertion creates one explicit Piano Roll block");
	auto firstRef = firstBrick.affected.front();
	Require(sndFile.Patterns[patternIndex].GetpModCommand(brickRow + 2, firstRef.channel)->note == NOTE_KEYOFF,
		"Insertion writes an explicit note-off");

	insertBrick.row = brickRow + 2;
	insertBrick.pitch = NOTE_MIDDLEC + 9;
	const auto secondBrick = editor.Apply(insertBrick);
	Require(secondBrick.applied && secondBrick.affected.size() == 1, "A touching note reuses the lowest instrument layer");
	auto secondRef = secondBrick.affected.front();
	Require(secondRef.channel == firstRef.channel
		&& sndFile.Patterns[patternIndex].GetpModCommand(brickRow + 2, firstRef.channel)->note == NOTE_MIDDLEC + 9,
		"A note start removes the coincident stop symbol");

	PianoRollPattern::Operation moveSecond;
	moveSecond.type = PianoRollPattern::OperationType::Move;
	moveSecond.pattern = patternIndex;
	moveSecond.notes = {secondRef};
	moveSecond.rowDelta = 1;
	const auto movedSecond = editor.Apply(moveSecond);
	Require(movedSecond.applied && movedSecond.affected.size() == 1, "A note can move later while preserving its duration");
	secondRef = movedSecond.affected.front();
	const auto afterMove = editor.Read(patternIndex);
	auto projectedFirst = std::find_if(afterMove.notes.begin(), afterMove.notes.end(), [&](const auto &note)
	{
		return note.id.row == brickRow && note.pitch == NOTE_MIDDLEC + 7;
	});
	Require(projectedFirst != afterMove.notes.end() && projectedFirst->endRow == brickRow + 2
		&& sndFile.Patterns[patternIndex].GetpModCommand(brickRow + 2, projectedFirst->id.channel)->note == NOTE_KEYOFF,
		"Moving the following note later fills the old implicit boundary with a stop symbol");

	PianoRollPattern::Operation lengthen;
	lengthen.type = PianoRollPattern::OperationType::Resize;
	lengthen.pattern = patternIndex;
	lengthen.notes = {projectedFirst->id};
	lengthen.length = 4;
	const auto lengthened = editor.Apply(lengthen);
	Require(lengthened.applied, "A note can be lengthened through a following note by adding an instrument layer");
	const auto layered = editor.Read(patternIndex);
	auto longNote = std::find_if(layered.notes.begin(), layered.notes.end(), [&](const auto &note) { return note.id.row == brickRow && note.pitch == NOTE_MIDDLEC + 7; });
	auto laterNote = std::find_if(layered.notes.begin(), layered.notes.end(), [&](const auto &note) { return note.id.row == brickRow + 3 && note.pitch == NOTE_MIDDLEC + 9; });
	Require(longNote != layered.notes.end() && laterNote != layered.notes.end() && longNote->id.channel != laterNote->id.channel,
		"Overlapping notes of one instrument occupy different layers");
	RequireInstrumentChannels(sndFile);

	PianoRollPattern::Operation removeLong;
	removeLong.type = PianoRollPattern::OperationType::Delete;
	removeLong.pattern = patternIndex;
	removeLong.notes = {longNote->id};
	Require(editor.Apply(removeLong).applied, "Deleting the lower brick repacks the instrument group");
	const auto fallen = editor.Read(patternIndex);
	auto fallenNote = std::find_if(fallen.notes.begin(), fallen.notes.end(), [&](const auto &note) { return note.id.row == brickRow + 3 && note.pitch == NOTE_MIDDLEC + 9; });
	CHANNELINDEX lowestInstrumentChannel = CHANNELINDEX_INVALID;
	for(const auto &note : fallen.notes) if(note.instrument == 1) lowestInstrumentChannel = std::min(lowestInstrumentChannel, note.id.channel);
	Require(fallenNote != fallen.notes.end() && fallenNote->id.channel == lowestInstrumentChannel,
		"When the lower brick is removed, the remaining brick falls to the lowest free layer");
	Require(sndFile.GetNumChannels() <= layered.channels,
		"Removing an overlap also removes any now-empty trailing Piano Roll layer");
	UndoOne(*document, "Repacking remains one atomic Undo step");

	document->SetModified(false);
	document->OnCloseDocument();
}

void PianoRollRealProjectTests(const CString &fixture)
{
	auto *document = static_cast<CModDoc *>(theApp.OpenDocumentFile(fixture, FALSE));
	if(document == nullptr)
		throw std::runtime_error("Cannot load Piano Roll real-project fixture");

	PianoRollPattern editor(*document);
	const auto &sndFile = document->GetSoundFile();
	std::vector<std::tuple<PATTERNINDEX, ROWINDEX, CHANNELINDEX, ModCommand>> trackerOnlyNotes;
	for(PATTERNINDEX pattern = 0; pattern < sndFile.Patterns.Size(); ++pattern)
	{
		if(!sndFile.Patterns.IsValidPat(pattern)) continue;
		for(CHANNELINDEX channel = 0; channel < sndFile.GetNumChannels(); ++channel)
			for(ROWINDEX row = 0; row < sndFile.Patterns[pattern].GetNumRows(); ++row)
			{
				const ModCommand cell = *sndFile.Patterns[pattern].GetpModCommand(row, channel);
				// A leading key-off cannot be a duration marker for a projected
				// note in this Pattern and must stay in its Tracker channel.
				if(cell.note >= NOTE_MIN_SPECIAL && (cell.note != NOTE_KEYOFF || row == 0))
					trackerOnlyNotes.emplace_back(pattern, row, channel, cell);
			}
	}
	Require(!trackerOnlyNotes.empty(), "The real-project regression fixture contains Tracker-only special notes");
	std::optional<PianoRollPattern::Note> editableNote;
	PianoRollPattern::Projection projection;
	for(PATTERNINDEX pattern = 0; pattern < sndFile.Patterns.Size() && !editableNote; ++pattern)
	{
		if(!sndFile.Patterns.IsValidPat(pattern)) continue;
		projection = editor.Read(pattern);
		Require(projection.editable, "A loaded real-project Pattern is editable in the Piano Roll");
		for(const auto &note : projection.notes)
		{
			if(note.pitch > projection.noteMin || note.pitch < projection.noteMax)
			{
				editableNote = note;
				break;
			}
		}
	}
	Require(editableNote.has_value(), "The real project exposes a pitched Piano Roll note");

	PianoRollPattern::Operation transpose;
	transpose.type = PianoRollPattern::OperationType::Transpose;
	transpose.pattern = editableNote->id.pattern;
	transpose.notes = {editableNote->id};
	transpose.pitchDelta = editableNote->pitch < projection.noteMax ? 1 : -1;
	const auto result = editor.Apply(transpose);
	if(!result.applied)
		throw std::runtime_error("A real-project Piano Roll note edit failed: " + mpt::ToCharset(mpt::Charset::UTF8, result.reason));
	Require(result.affected.size() == 1, "A single real-project note edit affects one Piano Roll note");
	const auto changed = editor.Read(result.affected.front().pattern);
	const auto changedNote = std::find_if(changed.notes.begin(), changed.notes.end(), [&](const auto &note)
	{
		return note.id == result.affected.front();
	});
	Require(changedNote != changed.notes.end() && changedNote->pitch == editableNote->pitch + transpose.pitchDelta,
		"The requested real-project pitch edit is visible in the Piano Roll projection");
	for(const auto &[pattern, row, channel, before] : trackerOnlyNotes)
		Require(*sndFile.Patterns[pattern].GetpModCommand(row, channel) == before,
			"Piano Roll editing preserves Tracker-only special notes in place");
	UndoOne(*document, "The real-project Piano Roll edit remains one atomic Undo step");
	const auto restored = editor.Read(editableNote->id.pattern);
	const auto restoredNote = std::find_if(restored.notes.begin(), restored.notes.end(), [&](const auto &note)
	{
		return note.id == editableNote->id;
	});
	Require(restoredNote != restored.notes.end() && restoredNote->pitch == editableNote->pitch,
		"Undo restores the real-project Piano Roll note");

	document->SetModified(false);
	document->OnCloseDocument();
}

}  // namespace Test

OPENMPT_NAMESPACE_END

#endif
