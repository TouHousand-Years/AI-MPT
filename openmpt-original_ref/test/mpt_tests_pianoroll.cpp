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

ROWINDEX FindCompletelyEmptyRow(const CSoundFile &sndFile, PATTERNINDEX pattern)
{
	const CPattern &source = sndFile.Patterns[pattern];
	for(ROWINDEX row = 0; row < source.GetNumRows(); ++row)
	{
		bool empty = true;
		for(CHANNELINDEX channel = 0; channel < sndFile.GetNumChannels(); ++channel)
			empty = empty && IsEmpty(*source.GetpModCommand(row, channel));
		if(empty)
			return row;
	}
	throw std::runtime_error("The Piano Roll fixture has no completely empty row");
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
	first.note = static_cast<ModCommand::NOTE>(NOTE_MIDDLEC);
	first.instr = 1;
	explicitOff.command = CMD_DELAYCUT;
	explicitOff.param = 1;
	const auto effectProjection = editor.Read(patternIndex);
	const auto effectNote = std::find_if(effectProjection.notes.begin(), effectProjection.notes.end(), [&](const auto &note)
	{
		return note.id == PianoRollPattern::NoteRef{patternIndex, empty.row, empty.channel};
	});
	Require(effectNote != effectProjection.notes.end() && effectNote->endRow == empty.row + 3,
		"Projection derives a Tracker effect-column termination");
	first = savedFirst;
	explicitOff = savedOff;

	// A pure effect / nonstandard-volume target is editable, and untouched
	// fields must survive the ordinary Pattern Undo cycle exactly.
	const ModCommand baseline = first;
	first.command = CMD_TEMPO;
	first.param = 125;
	first.volcmd = VOLCMD_PANNING;
	first.vol = 32;
	const auto draw = editor.Apply(InsertAt(patternIndex, empty.row, empty.channel, NOTE_MIDDLEC));
	Require(draw.applied && first.note == NOTE_MIDDLEC && first.instr == 1
		&& first.command == CMD_TEMPO && first.param == 125 && first.volcmd == VOLCMD_PANNING && first.vol == 32,
		"Drawing preserves effect and nonstandard volume fields");
	PianoRollPattern::Operation erase;
	erase.type = PianoRollPattern::OperationType::Delete;
	erase.pattern = patternIndex;
	erase.notes = draw.affected;
	Require(editor.Apply(erase).applied && first.note == NOTE_NONE && first.instr == 0
		&& first.command == CMD_TEMPO && first.param == 125 && first.volcmd == VOLCMD_PANNING && first.vol == 32,
		"Deleting clears only Piano Roll-owned fields");
	UndoOne(*document, "Delete produces one normal Pattern Undo step");
	UndoOne(*document, "Draw produces one normal Pattern Undo step");
	Require(first.note == NOTE_NONE && first.instr == 0 && first.volcmd == VOLCMD_PANNING && first.vol == 32
		&& first.command == CMD_TEMPO && first.param == 125, "Undo restores byte-preserved fields");
	first = baseline;

	const auto resizeStart = FindEmptySpan(sndFile, patternIndex, 8);
	const auto resizeDraw = editor.Apply(InsertAt(patternIndex, resizeStart.row, resizeStart.channel, NOTE_MIDDLEC + 2));
	Require(resizeDraw.applied, "A note can be drawn before resizing");
	PianoRollPattern::Operation resize;
	resize.type = PianoRollPattern::OperationType::Resize;
	resize.pattern = patternIndex;
	resize.notes = resizeDraw.affected;
	resize.length = 4;
	Require(editor.Apply(resize).applied && sndFile.Patterns[patternIndex].GetpModCommand(resizeStart.row + 4, resizeStart.channel)->note == NOTE_KEYOFF,
		"Resizing writes an exact note-off event");
	resize.length = 6;
	Require(editor.Apply(resize).applied && sndFile.Patterns[patternIndex].GetpModCommand(resizeStart.row + 4, resizeStart.channel)->note == NOTE_NONE
		&& sndFile.Patterns[patternIndex].GetpModCommand(resizeStart.row + 6, resizeStart.channel)->note == NOTE_KEYOFF,
		"An explicit note-off can be lengthened without crossing Tracker data");
	UndoOne(*document, "Lengthen produces one normal Pattern Undo step");
	UndoOne(*document, "Shorten produces one normal Pattern Undo step");
	UndoOne(*document, "Resize setup draw produces one normal Pattern Undo step");

	const auto leftDraw = editor.Apply(InsertAt(patternIndex, resizeStart.row + 2, resizeStart.channel, NOTE_MIDDLEC + 3));
	Require(leftDraw.applied, "A note can be drawn before moving its left edge");
	PianoRollPattern::Operation leftResize;
	leftResize.type = PianoRollPattern::OperationType::Resize;
	leftResize.pattern = patternIndex;
	leftResize.notes = leftDraw.affected;
	leftResize.resizeFromLeft = true;
	leftResize.rowDelta = -1;
	Require(editor.Apply(leftResize).applied && sndFile.Patterns[patternIndex].GetpModCommand(resizeStart.row + 1, resizeStart.channel)->note == NOTE_MIDDLEC + 3
		&& sndFile.Patterns[patternIndex].GetpModCommand(resizeStart.row + 2, resizeStart.channel)->note == NOTE_NONE,
		"Moving the left edge preserves the right endpoint");
	UndoOne(*document, "Left-edge resize produces one normal Pattern Undo step");
	UndoOne(*document, "Left-edge setup draw produces one normal Pattern Undo step");

	const auto moveStart = FindEmptySpan(sndFile, patternIndex, 8);
	const auto moveDraw = editor.Apply(InsertAt(patternIndex, moveStart.row, moveStart.channel, NOTE_MIDDLEC + 5));
	Require(moveDraw.applied, "A note can be drawn before transpose and move");
	PianoRollPattern::Operation transpose;
	transpose.type = PianoRollPattern::OperationType::Transpose;
	transpose.pattern = patternIndex;
	transpose.notes = moveDraw.affected;
	transpose.pitchDelta = 1;
	Require(editor.Apply(transpose).applied && sndFile.Patterns[patternIndex].GetpModCommand(moveStart.row, moveStart.channel)->note == NOTE_MIDDLEC + 6,
		"Transposition changes only the pitched note field");
	UndoOne(*document, "Transpose produces one normal Pattern Undo step");
	PianoRollPattern::Operation move;
	move.type = PianoRollPattern::OperationType::Move;
	move.pattern = patternIndex;
	move.notes = moveDraw.affected;
	move.rowDelta = 1;
	move.pitchDelta = 2;
	Require(editor.Apply(move).applied && sndFile.Patterns[patternIndex].GetpModCommand(moveStart.row, moveStart.channel)->note == NOTE_NONE
		&& sndFile.Patterns[patternIndex].GetpModCommand(moveStart.row + 1, moveStart.channel)->note == NOTE_MIDDLEC + 7,
		"Move uses a single transactional Pattern edit");
	UndoOne(*document, "Move produces one normal Pattern Undo step");
	UndoOne(*document, "Move setup draw produces one normal Pattern Undo step");

	const auto pasteRow = FindCompletelyEmptyRow(sndFile, patternIndex);
	PianoRollPattern::Operation paste;
	paste.type = PianoRollPattern::OperationType::Paste;
	paste.pattern = patternIndex;
	paste.row = pasteRow;
	paste.channel = 0;
	paste.pitch = NOTE_MIDDLEC;
	paste.clipboard = {{0, 0, 0, 1, 32}, {2, 4, 1, 1, std::nullopt}};
	Require(editor.Apply(paste).applied && sndFile.Patterns[patternIndex].GetpModCommand(pasteRow, 0)->note == NOTE_MIDDLEC
		&& sndFile.Patterns[patternIndex].GetpModCommand(pasteRow + 2, 1)->note == NOTE_MIDDLEC + 4,
		"Relative Piano Roll clipboard paste commits as one transaction");
	UndoOne(*document, "Multi-note paste produces one normal Pattern Undo step");

	const auto targetRow = FindCompletelyEmptyRow(sndFile, patternIndex);
	const CHANNELINDEX originalChannels = sndFile.GetNumChannels();
	std::vector<ModCommand> occupied;
	for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
	{
		auto &cell = *sndFile.Patterns[patternIndex].GetpModCommand(targetRow, channel);
		occupied.push_back(cell);
		cell.instr = 1;
	}
	const auto addedChannel = editor.Apply(InsertAt(patternIndex, targetRow, 0, NOTE_MIDDLEC + 4));
	Require(addedChannel.applied && addedChannel.channelsAdded && sndFile.GetNumChannels() == originalChannels + 1,
		"A full visible row allocates one channel atomically");
	UndoOne(*document, "Channel allocation is one normal Pattern Undo step");
	Require(sndFile.GetNumChannels() == originalChannels, "Undo restores the original channel layout");
	for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
		*sndFile.Patterns[patternIndex].GetpModCommand(targetRow, channel) = occupied[channel];

	// Special Tracker note cells are never valid graphical targets, even when
	// other channel allocation would otherwise be possible.
	for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
	{
		ModCommand special;
		special.note = channel & 1 ? NOTE_PC : NOTE_PCS;
		special.instr = 1;
		*sndFile.Patterns[patternIndex].GetpModCommand(targetRow, channel) = special;
	}
	const auto protectedCell = editor.Apply(InsertAt(patternIndex, targetRow, 0, NOTE_MIDDLEC + 5));
	Require(!protectedCell.applied && sndFile.GetNumChannels() == originalChannels,
		"PC and PCS cells reject a graphical edit atomically");
	for(CHANNELINDEX channel = 0; channel < originalChannels; ++channel)
		*sndFile.Patterns[patternIndex].GetpModCommand(targetRow, channel) = occupied[channel];

	auto invalid = InsertAt(patternIndex, empty.row, empty.channel, NOTE_MIDDLEC);
	invalid.instrument = 0;
	Require(!editor.Apply(invalid).applied, "Drawing rejects a missing instrument or sample");
	AI::PatternCapability occupancy(*document, patternIndex, {});
	const auto token = occupancy.Call("get_pattern_context", {{"occupy", true}}).at("session");
	Require(!editor.Apply(InsertAt(patternIndex, empty.row, empty.channel, NOTE_MIDDLEC)).applied,
		"Retained AI occupancy makes Piano Roll edits read-only");
	occupancy.Call("abort_session", {{"session", token}});

	document->SetModified(false);
	document->OnCloseDocument();
}

}  // namespace Test

OPENMPT_NAMESPACE_END

#endif
