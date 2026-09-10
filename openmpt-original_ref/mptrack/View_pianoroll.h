/* Piano Roll lower canvas. */
#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "Globals.h"
#include "PianoRollPattern.h"

OPENMPT_NAMESPACE_BEGIN

class CCtrlPianoRoll;

class CViewPianoRoll final : public CModScrollView
{
private:
	static constexpr int KeyboardWidth = 92;
	static constexpr int RulerHeight = 24;

	std::unique_ptr<PianoRollPattern> m_model;
	PATTERNINDEX m_pattern = 0;
	ROWINDEX m_cursorRow = 0;
	ModCommand::NOTE m_cursorPitch = NOTE_MIDDLEC;
	int m_rowWidth = 18, m_keyHeight = 18;
	std::vector<PianoRollPattern::NoteRef> m_selection;
	bool m_dragging = false, m_boxSelecting = false, m_boxAppend = false, m_resizing = false, m_resizeLeft = false;
	CPoint m_dragStart, m_dragNow;
	PianoRollPattern::NoteRef m_dragNote;
	CHANNELINDEX m_previewChannel = CHANNELINDEX_INVALID;
	ModCommand::NOTE m_previewPitch = NOTE_NONE;
	ROWINDEX m_playRow = ROWINDEX_INVALID;
	DWORD m_lastEmptyClickTime = 0;
	CPoint m_lastEmptyClickPoint{-10000, -10000};

public:
	CViewPianoRoll() = default;
	DECLARE_SERIAL(CViewPianoRoll)

	void OnInitialUpdate() override;
	void OnDraw(CDC *dc) override;
	void OnDPIChanged() override;
	void UpdateView(UpdateHint hint, CObject *pObject = nullptr) override;
	LRESULT OnModViewMsg(WPARAM wParam, LPARAM lParam) override;
	LRESULT OnPlayerNotify(Notification *notification) override;
	BOOL PreTranslateMessage(MSG *message) override;

private:
	CCtrlPianoRoll *GetPanel() const;
	PianoRollPattern::Projection Projection() const;
	void UpdateScrollSize();
	void PruneSelection(const PianoRollPattern::Projection &projection);
	CPoint ToVirtual(CPoint point) const;
	ROWINDEX RowAt(CPoint point, const PianoRollPattern::Projection &projection, bool snap = true) const;
	ModCommand::NOTE PitchAt(CPoint point, const PianoRollPattern::Projection &projection) const;
	CRect NoteRect(const PianoRollPattern::Note &note, const PianoRollPattern::Projection &projection) const;
	std::optional<PianoRollPattern::Note> HitNote(CPoint point, const PianoRollPattern::Projection &projection) const;
	bool IsSelected(const PianoRollPattern::NoteRef &note) const;
	void SelectOnly(const PianoRollPattern::NoteRef &note);
	void ToggleSelection(const PianoRollPattern::NoteRef &note);
	bool Commit(PianoRollPattern::Operation operation);
	void CopySelection(bool cut);
	void Paste();
	void SelectAll();
	void UndoRedo(bool undo);
	void PreviewPitch(ModCommand::NOTE pitch);
	void StopPreview();
	bool InsertAtPoint(CPoint point);
	void DrawNote(CDC &dc, const PianoRollPattern::Note &note, const PianoRollPattern::Projection &projection, const CPoint &delta = {}) const;
	void DrawSelectionBox(CDC &dc) const;

	void OnLButtonDown(UINT flags, CPoint point);
	void OnLButtonDblClk(UINT flags, CPoint point);
	void OnLButtonUp(UINT flags, CPoint point);
	void OnRButtonUp(UINT flags, CPoint point);
	void OnMouseMove(UINT flags, CPoint point);
	BOOL OnMouseWheel(UINT flags, short delta, CPoint point);
	BOOL OnScrollBy(CSize sizeScroll, BOOL doScroll = TRUE) override;
	BOOL OnEraseBkgnd(CDC *dc);
	void OnKeyDown(UINT key, UINT repeat, UINT flags);
	void OnEditUndo();
	void OnEditRedo();
	void OnEditCut();
	void OnEditCopy();
	void OnEditPaste();
	void OnEditSelectAll();
	void OnSplitChannels();
	LRESULT OnCustomKeyMsg(WPARAM command, LPARAM keyEvent);
	void OnSize(UINT type, int cx, int cy);
	void OnSetFocus(CWnd *oldWindow);
	void OnDestroy();
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
