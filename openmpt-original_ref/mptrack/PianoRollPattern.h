/*
 * PianoRollPattern.h
 * ------------------
 * The Piano Roll's deep module.  Pattern cells remain the only stored model;
 * this interface exposes a short-lived graphical projection and atomic edits.
 */

#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "../soundlib/modcommand.h"

#include <optional>

OPENMPT_NAMESPACE_BEGIN

class CModDoc;

class PianoRollPattern
{
public:
	struct ViewFilter
	{
		// An empty bitset means every existing channel is visible.  The filter is
		// view state, never a song property.
		std::vector<bool> visibleChannels;
		bool IsVisible(CHANNELINDEX channel, CHANNELINDEX channels) const;
	};

	struct NoteRef
	{
		PATTERNINDEX pattern = PATTERNINDEX_INVALID;
		ROWINDEX row = ROWINDEX_INVALID;
		CHANNELINDEX channel = CHANNELINDEX_INVALID;

		bool operator==(const NoteRef &other) const
		{
			return pattern == other.pattern && row == other.row && channel == other.channel;
		}
	};

	struct Note
	{
		NoteRef id;
		ModCommand::NOTE pitch = NOTE_NONE;
		ROWINDEX endRow = 0;  // exclusive; derived from Tracker termination data
		ModCommand::INSTR instrument = 0;
		std::optional<ModCommand::VOL> volume;
	};

	struct Projection
	{
		PATTERNINDEX pattern = PATTERNINDEX_INVALID;
		ROWINDEX rows = 0;
		CHANNELINDEX channels = 0;
		ROWINDEX rowsPerBeat = 0;
		ROWINDEX rowsPerMeasure = 0;
		ModCommand::NOTE noteMin = NOTE_MIN;
		ModCommand::NOTE noteMax = NOTE_MAX;
		bool editable = false;
		bool canResizeLength = false;
		CString nonEditableReason;
		CString resizeDisabledReason;
		std::vector<Note> notes;
	};

	struct ClipboardNote
	{
		ROWINDEX relativeRow = 0;
		int relativePitch = 0;
		CHANNELINDEX relativeChannel = 0;
		ModCommand::INSTR instrument = 0;
		std::optional<ModCommand::VOL> volume;
	};

	enum class OperationType : uint8
	{
		Insert,
		Delete,
		Move,
		Transpose,
		Resize,
		Paste,
	};

	struct Operation
	{
		OperationType type = OperationType::Insert;
		PATTERNINDEX pattern = PATTERNINDEX_INVALID;
		ViewFilter viewFilter;
		std::vector<NoteRef> notes;

		// Insert and paste origins, and the active channel used for conflict
		// routing. Move / transpose use the relative deltas below.
		ROWINDEX row = 0;
		CHANNELINDEX channel = 0;
		int pitch = NOTE_MIN;
		int rowDelta = 0;
		int pitchDelta = 0;
		ROWINDEX length = 1;
		bool resizeFromLeft = false;
		ModCommand::INSTR instrument = 0;
		std::optional<ModCommand::VOL> volume;
		std::vector<ClipboardNote> clipboard;
		CString undoName;
	};

	struct EditResult
	{
		bool applied = false;
		bool channelsAdded = false;
		CHANNELINDEX channels = 0;
		CString reason;
		std::vector<NoteRef> affected;
	};

	explicit PianoRollPattern(CModDoc &document) : m_document(document) {}

	Projection Read(PATTERNINDEX pattern, const ViewFilter &viewFilter = {}) const;
	EditResult Apply(const Operation &operation);

private:
	CModDoc &m_document;
};

OPENMPT_NAMESPACE_END
