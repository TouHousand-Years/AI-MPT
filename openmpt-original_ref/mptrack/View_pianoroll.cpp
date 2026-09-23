/* Piano Roll lower canvas. */

#include "stdafx.h"
#include "View_pianoroll.h"

#include "Childfrm.h"
#include "Ctrl_pianoroll.h"
#include "HighDPISupport.h"
#include "InputHandler.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Notification.h"
#include "TrackerSettings.h"
#include "WindowMessages.h"
#include "../soundlib/mod_specifications.h"

#include <set>

OPENMPT_NAMESPACE_BEGIN

namespace
{
// Deliberately separate from PatternClipboard and the system clipboard.  This
// contains only the Piano Roll's relative Tracker projection fields.
std::vector<PianoRollPattern::ClipboardNote> g_pianoRollClipboard;

bool IsBlackKey(ModCommand::NOTE note)
{
	switch((note - NOTE_MIN) % 12)
	{
	case 1: case 3: case 6: case 8: case 10: return true;
	default: return false;
	}
}

CString NoteLabel(ModCommand::NOTE note)
{
	static constexpr std::array<LPCTSTR, 12> names = {
		_T("C"), _T("C#"), _T("D"), _T("D#"), _T("E"), _T("F"),
		_T("F#"), _T("G"), _T("G#"), _T("A"), _T("A#"), _T("B")};
	const int index = note - NOTE_MIN;
	CString text;
	text.Format(_T("%s%d"), names[index % 12], index / 12);
	return text;
}

COLORREF InstrumentColour(ModCommand::INSTR instrument)
{
	static constexpr COLORREF palette[] = {
		RGB(0x55, 0xB7, 0xE8), RGB(0xF3, 0x9C, 0x3D), RGB(0x87, 0xC5, 0x55), RGB(0xC4, 0x79, 0xD9),
		RGB(0xE4, 0x68, 0x76), RGB(0x4F, 0xC3, 0xA1), RGB(0xD6, 0xB4, 0x4C), RGB(0x75, 0x8F, 0xDB)};
	return palette[(instrument ? instrument - 1 : 0) % std::size(palette)];
}

}  // namespace

IMPLEMENT_SERIAL(CViewPianoRoll, CModScrollView, 0)

BEGIN_MESSAGE_MAP(CViewPianoRoll, CModScrollView)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
	ON_WM_KEYDOWN()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_WM_DESTROY()
	ON_COMMAND(ID_EDIT_UNDO, &CViewPianoRoll::OnEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &CViewPianoRoll::OnEditRedo)
	ON_COMMAND(ID_EDIT_CUT, &CViewPianoRoll::OnEditCut)
	ON_COMMAND(ID_EDIT_COPY, &CViewPianoRoll::OnEditCopy)
	ON_COMMAND(ID_EDIT_PASTE, &CViewPianoRoll::OnEditPaste)
	ON_COMMAND(ID_EDIT_SELECT_ALL, &CViewPianoRoll::OnEditSelectAll)
	ON_COMMAND(IDC_PIANOROLL_SPLITCHANNELS, &CViewPianoRoll::OnSplitChannels)
	ON_MESSAGE(WM_MOD_KEYCOMMAND, &CViewPianoRoll::OnCustomKeyMsg)
END_MESSAGE_MAP()

CCtrlPianoRoll *CViewPianoRoll::GetPanel() const
{
	return dynamic_cast<CCtrlPianoRoll *>(const_cast<CViewPianoRoll *>(this)->GetControlDlg());
}

bool CViewPianoRoll::AllowEditing() const
{
	const CCtrlPianoRoll *panel = GetPanel();
	return panel && panel->AllowEditing();
}

void CViewPianoRoll::OnInitialUpdate()
{
	CModScrollView::OnInitialUpdate();
	m_model = std::make_unique<PianoRollPattern>(*GetDocument());
	if(CCtrlPianoRoll *panel = GetPanel()) m_pattern = panel->GetCurrentPattern();
	UpdateScrollSize();
}

PianoRollPattern::Projection CViewPianoRoll::Projection() const
{
	if(!m_model) return {};
	if(CCtrlPianoRoll *panel = GetPanel()) return m_model->Read(m_pattern, panel->GetViewFilter());
	return m_model->Read(m_pattern);
}

void CViewPianoRoll::UpdateScrollSize()
{
	const auto projection = Projection();
	const int pitches = std::max(1, int(projection.noteMax) - int(projection.noteMin) + 1);
	SetScrollSizes(MM_TEXT, CSize(KeyboardWidth + std::max<ROWINDEX>(1, projection.rows) * m_rowWidth,
		RulerHeight + pitches * m_keyHeight), CSize(m_rowWidth * 8, m_keyHeight * 8), CSize(m_rowWidth, m_keyHeight));
}

void CViewPianoRoll::PruneSelection(const PianoRollPattern::Projection &projection)
{
	std::set<std::tuple<PATTERNINDEX, ROWINDEX, CHANNELINDEX>> existing;
	for(const auto &note : projection.notes) existing.emplace(note.id.pattern, note.id.row, note.id.channel);
	m_selection.erase(std::remove_if(m_selection.begin(), m_selection.end(), [&](const auto &note)
	{
		return !existing.count({note.pattern, note.row, note.channel});
	}), m_selection.end());
}

CPoint CViewPianoRoll::ToVirtual(CPoint point) const { return point + GetScrollPosition(); }

ROWINDEX CViewPianoRoll::RowAt(CPoint point, const PianoRollPattern::Projection &projection, bool snap) const
{
	if(projection.rows == 0) return 0;
	const int raw = std::max(0, int((ToVirtual(point).x - KeyboardWidth) / std::max(1, m_rowWidth)));
	ROWINDEX row = static_cast<ROWINDEX>(std::min(raw, int(projection.rows - 1)));
	if(snap)
	{
		if(CCtrlPianoRoll *panel = GetPanel(); panel && panel->SnapEnabled())
		{
			const ROWINDEX spacing = std::max<ROWINDEX>(1, panel->GetSnapRows());
			row = static_cast<ROWINDEX>((row / spacing) * spacing);
		}
	}
	return row;
}

ModCommand::NOTE CViewPianoRoll::PitchAt(CPoint point, const PianoRollPattern::Projection &projection) const
{
	const int key = std::max(0, int((ToVirtual(point).y - RulerHeight) / std::max(1, m_keyHeight)));
	return static_cast<ModCommand::NOTE>(std::clamp<int>(int(projection.noteMax) - key, projection.noteMin, projection.noteMax));
}

CRect CViewPianoRoll::NoteRect(const PianoRollPattern::Note &note, const PianoRollPattern::Projection &projection) const
{
	const int left = KeyboardWidth + note.id.row * m_rowWidth;
	const int top = RulerHeight + (int(projection.noteMax) - int(note.pitch)) * m_keyHeight + 1;
	return CRect(left + 1, top, left + std::max<ROWINDEX>(1, note.endRow - note.id.row) * m_rowWidth - 1, top + m_keyHeight - 2);
}

std::optional<PianoRollPattern::Note> CViewPianoRoll::HitNote(CPoint point, const PianoRollPattern::Projection &projection) const
{
	const CPoint virtualPoint = ToVirtual(point);
	for(auto it = projection.notes.rbegin(); it != projection.notes.rend(); ++it)
		if(NoteRect(*it, projection).PtInRect(virtualPoint)) return *it;
	return {};
}

bool CViewPianoRoll::IsSelected(const PianoRollPattern::NoteRef &note) const
{
	return std::find(m_selection.begin(), m_selection.end(), note) != m_selection.end();
}

void CViewPianoRoll::SelectOnly(const PianoRollPattern::NoteRef &note)
{
	m_selection.assign(1, note);
}

void CViewPianoRoll::ToggleSelection(const PianoRollPattern::NoteRef &note)
{
	if(auto found = std::find(m_selection.begin(), m_selection.end(), note); found != m_selection.end())
		m_selection.erase(found);
	else
		m_selection.push_back(note);
}

bool CViewPianoRoll::Commit(PianoRollPattern::Operation operation)
{
	if(!AllowEditing() || !m_model) return false;
	if(CCtrlPianoRoll *panel = GetPanel()) operation.viewFilter = panel->GetViewFilter();
	const auto result = m_model->Apply(operation);
	if(result.applied)
	{
		m_selection = result.affected;
		PruneSelection(Projection());
		UpdateScrollSize();
		Invalidate(FALSE);
		return true;
	} else if(!result.reason.IsEmpty())
	{
		UpdateIndicator(result.reason);
	}
	return false;
}

void CViewPianoRoll::DrawNote(CDC &dc, const PianoRollPattern::Note &note, const PianoRollPattern::Projection &projection, const CPoint &delta) const
{
	CRect rect = NoteRect(note, projection);
	rect.OffsetRect(delta.x * m_rowWidth, -delta.y * m_keyHeight);
	const COLORREF colour = InstrumentColour(note.instrument);
	dc.FillSolidRect(rect, colour);
	dc.Draw3dRect(rect, RGB(245, 245, 248), RGB(28, 28, 32));
	if(rect.Width() > 34)
	{
		CString label = GetDocument()->GetPatternViewInstrumentName(note.instrument);
		const auto &specs = GetDocument()->GetSoundFile().GetModSpecifications();
		if(note.volumeCommand != VOLCMD_NONE)
			label.AppendFormat(_T("  %c%02X"), specs.GetVolEffectLetter(note.volumeCommand), note.volumeParameter);
		if(note.effectCommand != CMD_NONE)
			label.AppendFormat(_T("  %c%02X"), specs.GetEffectLetter(note.effectCommand), note.effectParameter);
		dc.SetBkMode(TRANSPARENT);
		const int luminance = GetRValue(colour) * 299 + GetGValue(colour) * 587 + GetBValue(colour) * 114;
		dc.SetTextColor(luminance >= 145000 ? RGB(20, 20, 22) : RGB(250, 250, 252));
		CRect labelRect = rect;
		labelRect.DeflateRect(3, 0);
		dc.DrawText(label, labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
	}
	if(IsSelected(note.id))
	{
		// A high-contrast double inner stroke stays visible against every
		// instrument colour and does not change the note's apparent duration.
		CRect selection = rect;
		selection.DeflateRect(2, 2);
		if(selection.Width() > 4 && selection.Height() > 4)
		{
			dc.Draw3dRect(selection, RGB(255, 255, 255), RGB(255, 255, 255));
			selection.DeflateRect(1, 1);
			dc.Draw3dRect(selection, RGB(35, 35, 40), RGB(35, 35, 40));
		}
	}
}

void CViewPianoRoll::DrawSelectionBox(CDC &dc) const
{
	if(!m_boxSelecting) return;
	CRect box(ToVirtual(m_dragStart), ToVirtual(m_dragNow));
	box.NormalizeRect();
	CBrush hatch(HS_DIAGCROSS, RGB(180, 180, 220));
	dc.FrameRect(box, &hatch);
}

void CViewPianoRoll::OnDraw(CDC *dc)
{
	if(!dc) return;
	const auto projection = Projection();
	PruneSelection(projection);
	const CPoint scroll = GetScrollPosition();
	CRect client;
	GetClientRect(client);
	if(client.IsRectEmpty()) return;
	const int right = scroll.x + client.Width();
	const int bottom = scroll.y + client.Height();
	const int gridRight = KeyboardWidth + projection.rows * m_rowWidth;
	const int pitches = std::max(1, int(projection.noteMax) - int(projection.noteMin) + 1);
	const int gridBottom = RulerHeight + pitches * m_keyHeight;
	const int contentLeft = scroll.x + KeyboardWidth;
	const int contentTop = scroll.y + RulerHeight;
	const int visibleGridRight = std::min(right, gridRight);
	const int visibleGridBottom = std::min(bottom, gridBottom);
	const auto &colours = TrackerSettings::Instance().rgbCustomColors;
	const COLORREF background = colours[MODCOLOR_BACKNORMAL];

	// CModScrollView intentionally does not offset its paint DC. Draw into a
	// client-sized back buffer with a virtual-coordinate origin so scrolling
	// cannot expose stale pixels and drag previews do not flicker.
	CDC buffer;
	CBitmap bitmap;
	if(!buffer.CreateCompatibleDC(dc) || !bitmap.CreateCompatibleBitmap(dc, client.Width(), client.Height())) return;
	CBitmap *oldBitmap = buffer.SelectObject(&bitmap);
	CFont *oldFont = buffer.SelectObject(dc->GetCurrentFont());
	buffer.SetViewportOrg(-scroll.x, -scroll.y);
	buffer.SetBkMode(TRANSPARENT);
	buffer.FillSolidRect(CRect(scroll.x, scroll.y, right, bottom), background);

	if(contentLeft < visibleGridRight && contentTop < visibleGridBottom)
	{
		const int saved = buffer.SaveDC();
		buffer.IntersectClipRect(CRect(contentLeft, contentTop, visibleGridRight, visibleGridBottom));
		for(ROWINDEX row = 0; row <= projection.rows; ++row)
		{
			const int x = KeyboardWidth + row * m_rowWidth;
			if(x < contentLeft || x > visibleGridRight) continue;
			const bool measure = projection.rowsPerMeasure && row % projection.rowsPerMeasure == 0;
			const bool beat = projection.rowsPerBeat && row % projection.rowsPerBeat == 0;
			const COLORREF line = measure ? RGB(158, 158, 166) : beat ? RGB(190, 190, 198) : RGB(220, 220, 226);
			buffer.FillSolidRect(CRect(x, RulerHeight, x + (measure ? 2 : 1), gridBottom), line);
		}
		for(int pitch = projection.noteMax; pitch >= projection.noteMin; --pitch)
		{
			const int y = RulerHeight + (int(projection.noteMax) - pitch + 1) * m_keyHeight - 1;
			if(y >= contentTop && y < visibleGridBottom)
				buffer.FillSolidRect(CRect(KeyboardWidth, y, gridRight, y + 1), RGB(216, 216, 222));
		}
		if(m_playRow < projection.rows)
		{
			const int x = KeyboardWidth + m_playRow * m_rowWidth;
			buffer.FillSolidRect(CRect(x, RulerHeight, x + 2, gridBottom), RGB(244, 82, 82));
		}
		if(m_cursorRow < projection.rows)
		{
			const int x = KeyboardWidth + m_cursorRow * m_rowWidth;
			buffer.FillSolidRect(CRect(x, RulerHeight, x + 1, gridBottom), RGB(220, 196, 72));
		}
		for(const auto &note : projection.notes)
			DrawNote(buffer, note, projection);
		if(m_dragging && !m_resizing)
		{
			const int rowDelta = int(RowAt(m_dragNow, projection)) - int(RowAt(m_dragStart, projection));
			const int pitchDelta = int(PitchAt(m_dragNow, projection)) - int(PitchAt(m_dragStart, projection));
			for(const auto &note : projection.notes)
				if(IsSelected(note.id)) DrawNote(buffer, note, projection, CPoint(rowDelta, pitchDelta));
		}
		if(m_dragging && m_resizing)
		{
			for(const auto &note : projection.notes) if(note.id == m_dragNote)
			{
				auto preview = note;
				if(m_resizeLeft)
					preview.id.row = std::min<ROWINDEX>(RowAt(m_dragNow, projection), static_cast<ROWINDEX>(preview.endRow - 1));
				else
					preview.endRow = std::max<ROWINDEX>(static_cast<ROWINDEX>(preview.id.row + 1), static_cast<ROWINDEX>(RowAt(m_dragNow, projection) + 1));
				DrawNote(buffer, preview, projection);
			}
		}
		DrawSelectionBox(buffer);
		buffer.RestoreDC(saved);
	}

	// The ruler stays fixed vertically. Its text uses a high-contrast semibold
	// font instead of the tracker text colour, which may be dark in light themes.
	buffer.FillSolidRect(CRect(scroll.x, scroll.y, right, scroll.y + RulerHeight), RGB(45, 45, 50));
	buffer.SetTextColor(RGB(242, 242, 246));
	LOGFONT rulerLogFont{};
	CFont rulerFont;
	CFont *rulerOldFont = nullptr;
	if(CFont *font = buffer.GetCurrentFont(); font && font->GetLogFont(&rulerLogFont))
	{
		rulerLogFont.lfWeight = FW_SEMIBOLD;
		if(rulerFont.CreateFontIndirect(&rulerLogFont)) rulerOldFont = buffer.SelectObject(&rulerFont);
	}
	for(ROWINDEX row = 0; row <= projection.rows; ++row)
	{
		const int x = KeyboardWidth + row * m_rowWidth;
		if(x < contentLeft || x > visibleGridRight) continue;
		const bool measure = projection.rowsPerMeasure && row % projection.rowsPerMeasure == 0;
		const bool beat = projection.rowsPerBeat && row % projection.rowsPerBeat == 0;
		if(measure || (beat && m_rowWidth >= 12))
		{
			CString label;
			label.Format(_T("%u"), row);
			CRect labelRect(x + 3, scroll.y, std::min(x + std::max(m_rowWidth * 4, 28), visibleGridRight), scroll.y + RulerHeight);
			buffer.DrawText(label, labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
	}
	if(rulerOldFont) buffer.SelectObject(rulerOldFont);

	// The keyboard stays fixed horizontally but follows vertical scrolling.
	buffer.FillSolidRect(CRect(scroll.x, scroll.y + RulerHeight, scroll.x + KeyboardWidth, bottom), RGB(52, 52, 56));
	if(contentTop < visibleGridBottom)
	{
		const int saved = buffer.SaveDC();
		buffer.IntersectClipRect(CRect(scroll.x, contentTop, scroll.x + KeyboardWidth, visibleGridBottom));
		for(int pitch = projection.noteMax; pitch >= projection.noteMin; --pitch)
		{
			const int y = RulerHeight + (int(projection.noteMax) - pitch) * m_keyHeight;
			if(y + m_keyHeight <= contentTop || y >= visibleGridBottom) continue;
			const bool black = IsBlackKey(static_cast<ModCommand::NOTE>(pitch));
			buffer.FillSolidRect(CRect(scroll.x, y, scroll.x + KeyboardWidth, y + m_keyHeight), black ? RGB(35, 35, 40) : RGB(205, 205, 208));
			buffer.FillSolidRect(CRect(scroll.x, y + m_keyHeight - 1, scroll.x + KeyboardWidth, y + m_keyHeight), RGB(112, 112, 118));
			buffer.SetTextColor(black ? RGB(242, 242, 245) : RGB(24, 24, 26));
			CRect labelRect(scroll.x + 5, y, scroll.x + KeyboardWidth - 4, y + m_keyHeight);
			buffer.DrawText(NoteLabel(static_cast<ModCommand::NOTE>(pitch)), labelRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
		}
		buffer.RestoreDC(saved);
	}
	buffer.FillSolidRect(CRect(scroll.x, scroll.y, scroll.x + KeyboardWidth, scroll.y + RulerHeight), RGB(38, 38, 42));

	if(!projection.editable && !projection.nonEditableReason.IsEmpty())
	{
		buffer.SetTextColor(RGB(255, 180, 100));
		buffer.TextOut(scroll.x + KeyboardWidth + 8, scroll.y + RulerHeight + 8, projection.nonEditableReason);
	}
	if(!projection.canResizeLength && !projection.resizeDisabledReason.IsEmpty())
	{
		buffer.SetTextColor(RGB(255, 180, 100));
		buffer.TextOut(scroll.x + KeyboardWidth + 8, scroll.y + 3, projection.resizeDisabledReason);
	}

	buffer.SetViewportOrg(0, 0);
	dc->BitBlt(0, 0, client.Width(), client.Height(), &buffer, 0, 0, SRCCOPY);
	buffer.SelectObject(oldFont);
	buffer.SelectObject(oldBitmap);
}

void CViewPianoRoll::OnLButtonDown(UINT flags, CPoint point)
{
	SetFocus();
	const auto projection = Projection();
	if(projection.rows == 0) return;
	if(point.x < KeyboardWidth)
	{
		PreviewPitch(PitchAt(point, projection));
		return;
	}
	if(point.y < RulerHeight)
	{
		m_cursorRow = RowAt(point, projection, false);
		Invalidate(FALSE);
		return;
	}
	m_cursorRow = RowAt(point, projection);
	m_cursorPitch = PitchAt(point, projection);
	if(const auto hit = HitNote(point, projection))
	{
		if(flags & MK_CONTROL) ToggleSelection(hit->id); else if(!IsSelected(hit->id)) SelectOnly(hit->id);
		if(!AllowEditing())
		{
			Invalidate(FALSE);
			return;
		}
		m_dragging = true;
		m_dragStart = m_dragNow = point;
		m_dragNote = hit->id;
		const CRect rect = NoteRect(*hit, projection);
		const int edge = 4;
		m_resizeLeft = std::abs(ToVirtual(point).x - rect.left) <= edge;
		m_resizing = m_resizeLeft || std::abs(ToVirtual(point).x - rect.right) <= edge;
		SetCapture();
		Invalidate(FALSE);
		return;
	}
	if(flags & MK_SHIFT)
	{
		m_boxSelecting = true;
		m_boxAppend = (flags & MK_CONTROL) != 0;
		m_dragStart = m_dragNow = point;
		SetCapture();
		return;
	}
	// Some host window classes do not carry CS_DBLCLKS, in which case Windows
	// sends two button-down messages instead of WM_LBUTTONDBLCLK. Accept both
	// forms so empty-cell insertion is independent of the containing view class.
	const DWORD clickTime = GetMessageTime();
	const CSize doubleClickSize(GetSystemMetrics(SM_CXDOUBLECLK), GetSystemMetrics(SM_CYDOUBLECLK));
	if(m_lastEmptyClickTime != 0 && clickTime - m_lastEmptyClickTime <= GetDoubleClickTime()
		&& std::abs(point.x - m_lastEmptyClickPoint.x) <= doubleClickSize.cx
		&& std::abs(point.y - m_lastEmptyClickPoint.y) <= doubleClickSize.cy)
	{
		m_lastEmptyClickTime = 0;
		InsertAtPoint(point);
		return;
	}
	m_lastEmptyClickTime = clickTime;
	m_lastEmptyClickPoint = point;
	Invalidate(FALSE);
}

bool CViewPianoRoll::InsertAtPoint(CPoint point)
{
	if(!AllowEditing()) return false;
	const auto projection = Projection();
	if(point.x < KeyboardWidth || point.y < RulerHeight || HitNote(point, projection)) return false;
	CCtrlPianoRoll *panel = GetPanel();
	if(!panel || panel->GetSelectedInstrument() == 0)
	{
		UpdateIndicator(_T("Choose an existing instrument or sample before adding a note."));
		return false;
	}
	PianoRollPattern::Operation operation;
	operation.type = PianoRollPattern::OperationType::Insert;
	operation.pattern = m_pattern;
	operation.row = RowAt(point, projection);
	operation.pitch = PitchAt(point, projection);
	operation.instrument = panel->GetSelectedInstrument();
	operation.length = panel->SnapEnabled() ? panel->GetSnapRows() : 1;
	return Commit(std::move(operation));

}

void CViewPianoRoll::OnLButtonDblClk(UINT, CPoint point)
{
	m_lastEmptyClickTime = 0;
	InsertAtPoint(point);
}

void CViewPianoRoll::OnLButtonUp(UINT, CPoint point)
{
	if(m_previewPitch != NOTE_NONE) StopPreview();
	const auto projection = Projection();
	if(m_boxSelecting)
	{
		m_boxSelecting = false;
		CRect box(ToVirtual(m_dragStart), ToVirtual(point));
		box.NormalizeRect();
		if(!m_boxAppend) m_selection.clear();
		for(const auto &note : projection.notes)
		{
			CRect overlap;
			if(overlap.IntersectRect(box, NoteRect(note, projection)) && !IsSelected(note.id)) m_selection.push_back(note.id);
		}
		m_boxAppend = false;
	}
	if(m_dragging)
	{
		m_dragging = false;
		PianoRollPattern::Operation operation;
		operation.pattern = m_pattern;
		if(m_resizing)
		{
			operation.type = PianoRollPattern::OperationType::Resize;
			operation.notes = {m_dragNote};
			operation.resizeFromLeft = m_resizeLeft;
			if(m_resizeLeft)
				operation.rowDelta = int(RowAt(point, projection)) - int(m_dragNote.row);
			else
				operation.length = static_cast<ROWINDEX>(std::max(1, int(RowAt(point, projection)) - int(m_dragNote.row) + 1));
		} else
		{
			operation.type = PianoRollPattern::OperationType::Move;
			operation.notes = m_selection;
			operation.rowDelta = int(RowAt(point, projection)) - int(RowAt(m_dragStart, projection));
			operation.pitchDelta = int(PitchAt(point, projection)) - int(PitchAt(m_dragStart, projection));
		}
		m_resizing = false;
		m_resizeLeft = false;
		if(operation.rowDelta || operation.pitchDelta || operation.type == PianoRollPattern::OperationType::Resize) Commit(std::move(operation));
	}
	if(GetCapture() == this) ReleaseCapture();
	Invalidate(FALSE);
}

void CViewPianoRoll::OnRButtonUp(UINT, CPoint point)
{
	const auto projection = Projection();
	if(const auto hit = HitNote(point, projection))
	{
		if(!IsSelected(hit->id)) SelectOnly(hit->id);
		PianoRollPattern::Operation operation;
		operation.type = PianoRollPattern::OperationType::Delete;
		operation.pattern = m_pattern;
		operation.notes = m_selection;
		Commit(std::move(operation));
	}
}

void CViewPianoRoll::OnMouseMove(UINT, CPoint point)
{
	if(m_dragging || m_boxSelecting)
	{
		m_dragNow = point;
		Invalidate(FALSE);
	}
}

BOOL CViewPianoRoll::OnMouseWheel(UINT flags, short delta, CPoint point)
{
	if(flags & MK_CONTROL)
	{
		m_rowWidth = std::clamp(m_rowWidth + (delta > 0 ? 2 : -2), 6, 80);
		UpdateScrollSize();
		Invalidate(FALSE);
		return TRUE;
	}
	if(flags & MK_SHIFT)
	{
		m_keyHeight = std::clamp(m_keyHeight + (delta > 0 ? 1 : -1), 16, 40);
		UpdateScrollSize();
		Invalidate(FALSE);
		return TRUE;
	}
	return CModScrollView::OnMouseWheel(flags, delta, point);
}

BOOL CViewPianoRoll::OnScrollBy(CSize sizeScroll, BOOL doScroll)
{
	const BOOL moved = CModScrollView::OnScrollBy(sizeScroll, doScroll);
	if(moved && doScroll)
	{
		// CScrollView normally reuses shifted client pixels and repaints only the
		// newly exposed strip. That would shift our fixed ruler / keyboard into
		// the score, so every completed scroll must present a fresh back buffer.
		Invalidate(FALSE);
	}
	return moved;
}

BOOL CViewPianoRoll::OnEraseBkgnd(CDC *)
{
	// OnDraw always presents a complete back buffer.
	return TRUE;
}

void CViewPianoRoll::CopySelection(bool cut)
{
	if(cut && !AllowEditing()) return;
	const auto projection = Projection();
	if(m_selection.empty()) return;
	ROWINDEX firstRow = std::numeric_limits<ROWINDEX>::max();
	int firstPitch = NOTE_MAX;
	CHANNELINDEX firstChannel = std::numeric_limits<CHANNELINDEX>::max();
	for(const auto &note : projection.notes) if(IsSelected(note.id))
	{
		firstRow = std::min(firstRow, note.id.row);
		firstPitch = std::min(firstPitch, int(note.pitch));
		firstChannel = std::min(firstChannel, note.id.channel);
	}
	g_pianoRollClipboard.clear();
	for(const auto &note : projection.notes) if(IsSelected(note.id))
		g_pianoRollClipboard.push_back({static_cast<ROWINDEX>(note.id.row - firstRow), int(note.pitch) - firstPitch,
			static_cast<CHANNELINDEX>(note.id.channel - firstChannel), note.instrument, note.volume,
			static_cast<ROWINDEX>(note.endRow - note.id.row)});
	if(cut)
	{
		PianoRollPattern::Operation operation;
		operation.type = PianoRollPattern::OperationType::Delete;
		operation.pattern = m_pattern;
		operation.notes = m_selection;
		operation.undoName = _T("Piano Roll: Cut Notes");
		Commit(std::move(operation));
	}
}

void CViewPianoRoll::Paste()
{
	if(g_pianoRollClipboard.empty()) return;
	CCtrlPianoRoll *panel = GetPanel();
	if(!panel) return;
	PianoRollPattern::Operation operation;
	operation.type = PianoRollPattern::OperationType::Paste;
	operation.pattern = m_pattern;
	operation.row = m_cursorRow;
	operation.channel = 0;
	operation.pitch = m_cursorPitch;
	operation.clipboard = g_pianoRollClipboard;
	Commit(std::move(operation));
}

void CViewPianoRoll::SelectAll()
{
	m_selection.clear();
	for(const auto &note : Projection().notes) m_selection.push_back(note.id);
	Invalidate(FALSE);
}

void CViewPianoRoll::UndoRedo(bool undo)
{
	CModDoc *document = GetDocument();
	if(!document) return;
	const PATTERNINDEX pattern = undo ? document->GetPatternUndo().Undo() : document->GetPatternUndo().Redo();
	if(pattern == PATTERNINDEX_INVALID) return;
	m_selection.clear();
	if(document->GetSoundFile().Patterns.IsValidPat(pattern))
	{
		if(CCtrlPianoRoll *panel = GetPanel())
			panel->SetCurrentPattern(pattern);
		else
			m_pattern = pattern;
	}
	UpdateScrollSize();
	Invalidate(FALSE);
}

void CViewPianoRoll::OnEditUndo() { UndoRedo(true); }
void CViewPianoRoll::OnEditRedo() { UndoRedo(false); }
void CViewPianoRoll::OnEditCut() { CopySelection(true); }
void CViewPianoRoll::OnEditCopy() { CopySelection(false); }
void CViewPianoRoll::OnEditPaste() { Paste(); }
void CViewPianoRoll::OnEditSelectAll() { SelectAll(); }

void CViewPianoRoll::OnSplitChannels()
{
	PianoRollPattern::Operation operation;
	operation.type = PianoRollPattern::OperationType::NormalizeChannels;
	operation.pattern = m_pattern;
	Commit(std::move(operation));
}

LRESULT CViewPianoRoll::OnCustomKeyMsg(WPARAM command, LPARAM)
{
	switch(command)
	{
	case kcEditUndo: OnEditUndo(); break;
	case kcEditRedo: OnEditRedo(); break;
	case kcEditCut: OnEditCut(); break;
	case kcEditCopy: OnEditCopy(); break;
	case kcEditPaste: OnEditPaste(); break;
	case kcEditSelectAll: OnEditSelectAll(); break;
	default: return kcNull;
	}
	return command;
}

void CViewPianoRoll::OnKeyDown(UINT key, UINT, UINT)
{
	const bool control = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
	if(control && key == 'A')
	{
		SelectAll();
		return;
	}
	if(control && key == 'C') { CopySelection(false); return; }
	if(control && key == 'X') { CopySelection(true); return; }
	if(control && key == 'V') { Paste(); return; }
	if(key == VK_DELETE || key == VK_BACK)
	{
		PianoRollPattern::Operation operation;
		operation.type = PianoRollPattern::OperationType::Delete;
		operation.pattern = m_pattern;
		operation.notes = m_selection;
		Commit(std::move(operation));
		return;
	}
	if(key == VK_LEFT || key == VK_RIGHT || key == VK_UP || key == VK_DOWN)
	{
		PianoRollPattern::Operation operation;
		operation.type = (key == VK_UP || key == VK_DOWN) ? PianoRollPattern::OperationType::Transpose : PianoRollPattern::OperationType::Move;
		operation.pattern = m_pattern;
		operation.notes = m_selection;
		if(key == VK_LEFT) operation.rowDelta = -1;
		if(key == VK_RIGHT) operation.rowDelta = 1;
		if(key == VK_UP) operation.pitchDelta = 1;
		if(key == VK_DOWN) operation.pitchDelta = -1;
		Commit(std::move(operation));
		return;
	}
	// Compact FamiStudio-style computer-keyboard preview row.  It previews only;
	// recording remains deliberately outside this first Piano Roll page.
	static constexpr std::array<UINT, 12> keys = {'Z', 'S', 'X', 'D', 'C', 'V', 'G', 'B', 'H', 'N', 'J', 'M'};
	if(auto found = std::find(keys.begin(), keys.end(), key); found != keys.end())
		PreviewPitch(static_cast<ModCommand::NOTE>(NOTE_MIDDLEC + std::distance(keys.begin(), found)));
	else
		CModScrollView::OnKeyDown(key, 1, 0);
}

BOOL CViewPianoRoll::PreTranslateMessage(MSG *message)
{
	if(message && message->hwnd == m_hWnd && message->message == WM_LBUTTONDBLCLK)
	{
		// Consume the gesture before CScrollView or its parent can reinterpret it.
		// lParam is in this view's client coordinates for mouse messages.
		const CPoint point(static_cast<short>(LOWORD(message->lParam)), static_cast<short>(HIWORD(message->lParam)));
		m_lastEmptyClickTime = 0;
		InsertAtPoint(point);
		return TRUE;
	}
	if(message && message->message == WM_KEYUP && m_previewPitch != NOTE_NONE)
	{
		StopPreview();
		return TRUE;
	}
	return CModScrollView::PreTranslateMessage(message);
}

void CViewPianoRoll::PreviewPitch(ModCommand::NOTE pitch)
{
	CCtrlPianoRoll *panel = GetPanel();
	if(!panel || panel->GetSelectedInstrument() == 0 || !ModCommand::IsNote(pitch)) return;
	StopPreview();
	PlayNoteParam params(pitch);
	if(GetDocument()->GetSoundFile().GetNumInstruments() > 0) params.Instrument(panel->GetSelectedInstrument());
	else params.Sample(panel->GetSelectedInstrument());
	m_previewChannel = GetDocument()->PlayNote(params);
	m_previewPitch = pitch;
}

void CViewPianoRoll::StopPreview()
{
	if(m_previewPitch != NOTE_NONE)
	{
		const INSTRUMENTINDEX instrument = GetPanel() ? GetPanel()->GetSelectedInstrument() : INSTRUMENTINDEX_INVALID;
		GetDocument()->NoteOff(m_previewPitch, GetDocument()->GetSoundFile().GetNumInstruments() == 0, instrument, m_previewChannel);
	}
	m_previewPitch = NOTE_NONE;
	m_previewChannel = CHANNELINDEX_INVALID;
}

void CViewPianoRoll::OnSize(UINT type, int cx, int cy)
{
	CModScrollView::OnSize(type, cx, cy);
	if(cx > 0 && cy > 0) UpdateScrollSize();
}

void CViewPianoRoll::OnSetFocus(CWnd *oldWindow)
{
	CModScrollView::OnSetFocus(oldWindow);
	if(CModDoc *document = GetDocument()) document->SetNotifications(Notification::Position);
}

void CViewPianoRoll::OnDestroy()
{
	StopPreview();
	CModScrollView::OnDestroy();
}

void CViewPianoRoll::OnDPIChanged()
{
	m_rowWidth = std::max(6, HighDPISupport::ScalePixels(18, m_hWnd));
	m_keyHeight = std::max(16, HighDPISupport::ScalePixels(18, m_hWnd));
	UpdateScrollSize();
	Invalidate(FALSE);
}

void CViewPianoRoll::UpdateView(UpdateHint hint, CObject *)
{
	const PatternHint patternHint = hint.ToType<PatternHint>();
	if(hint.GetType()[HINT_MODTYPE] || (patternHint.GetType()[HINT_PATTERNDATA] && (!patternHint.GetPattern() || patternHint.GetPattern() == m_pattern))
		|| hint.GetCategory() == HINTCAT_GENERAL)
	{
		PruneSelection(Projection());
		UpdateScrollSize();
		Invalidate(FALSE);
	}
}

LRESULT CViewPianoRoll::OnModViewMsg(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case VIEWMSG_SETCTRLWND:
		m_hWndCtrl = reinterpret_cast<HWND>(lParam);
		if(CCtrlPianoRoll *panel = GetPanel()) m_pattern = panel->GetCurrentPattern();
		UpdateScrollSize();
		break;
	case VIEWMSG_SETCURRENTPATTERN:
		if(GetDocument()->GetSoundFile().Patterns.IsValidPat(static_cast<PATTERNINDEX>(lParam)))
		{
			m_pattern = static_cast<PATTERNINDEX>(lParam);
			m_selection.clear();
			UpdateScrollSize();
			Invalidate(FALSE);
		}
		break;
	case VIEWMSG_GETCURRENTPATTERN:
		return m_pattern;
	case VIEWMSG_FOLLOWSONG:
		if(lParam) GetDocument()->SetNotifications(Notification::Position);
		break;
	case VIEWMSG_SAVESTATE:
		if(auto *state = reinterpret_cast<PianoRollViewState *>(lParam))
		{
			state->initialized = true;
			state->nPattern = m_pattern;
			state->firstRow = static_cast<ROWINDEX>(GetScrollPosition().x / std::max(1, m_rowWidth));
			const auto projection = Projection();
			state->topPitch = std::clamp(int(projection.noteMax)
				- std::max(0, static_cast<int>(GetScrollPosition().y) / std::max(1, m_keyHeight)),
				int(projection.noteMin), int(projection.noteMax));
			state->rowZoom = m_rowWidth;
			state->keyZoom = m_keyHeight;
			if(CCtrlPianoRoll *panel = GetPanel())
			{
				state->activeChannel = 0;
				state->instrument = panel->GetSelectedInstrument();
				state->snap = panel->SnapEnabled();
				state->showAllChannels = panel->GetViewFilter().visibleChannels.empty();
				state->followSong = panel->GetFollowSong();
				state->snapRows = panel->GetSnapRows();
			}
			state->selection.clear();
			for(const auto &note : m_selection) state->selection.push_back({note.row, note.channel});
		}
		break;
	case VIEWMSG_LOADSTATE:
		if(auto *state = reinterpret_cast<PianoRollViewState *>(lParam); state && state->initialized)
		{
			m_pattern = state->nPattern;
			m_rowWidth = std::clamp(state->rowZoom, 6, 80);
			m_keyHeight = std::clamp(state->keyZoom, 16, 40);
			m_selection.clear();
			for(const auto &note : state->selection) m_selection.push_back({m_pattern, note.row, note.channel});
			UpdateScrollSize();
			const auto projection = Projection();
			const int topPitch = std::clamp(state->topPitch, int(projection.noteMin), int(projection.noteMax));
			const int vertical = (int(projection.noteMax) - topPitch) * m_keyHeight;
			ScrollToPosition(CPoint(state->firstRow * m_rowWidth, std::max(0, vertical)));
			PruneSelection(projection);
			Invalidate(FALSE);
		}
		break;
	default:
		return CModScrollView::OnModViewMsg(wParam, lParam);
	}
	return 0;
}

LRESULT CViewPianoRoll::OnPlayerNotify(Notification *notification)
{
	if(!notification || !notification->type[Notification::Position]) return 0;
	if(notification->pattern == m_pattern)
	{
		m_playRow = notification->row;
		if(CCtrlPianoRoll *panel = GetPanel(); panel && panel->GetFollowSong())
		{
			CRect client;
			GetClientRect(client);
			const int target = KeyboardWidth + m_playRow * m_rowWidth - client.Width() / 3;
			ScrollToPosition(CPoint(std::max(0, target), GetScrollPosition().y));
		}
		Invalidate(FALSE);
	}
	return 0;
}

OPENMPT_NAMESPACE_END
