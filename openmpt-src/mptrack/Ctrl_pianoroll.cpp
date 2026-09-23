/* Piano Roll upper control panel. */

#include "stdafx.h"
#include "Ctrl_pianoroll.h"

#include "Childfrm.h"
#include "Mainfrm.h"
#include "Moddoc.h"
#include "Mptrack.h"
#include "resource.h"
#include "Reporting.h"
#include "TrackerSettings.h"
#include "View_pianoroll.h"
#include "WindowMessages.h"

OPENMPT_NAMESPACE_BEGIN

BEGIN_MESSAGE_MAP(CCtrlPianoRoll, CModControlDlg)
	ON_CBN_SELENDOK(IDC_PIANOROLL_PATTERN, &CCtrlPianoRoll::OnPatternChanged)
	ON_CBN_SELENDOK(IDC_PIANOROLL_INSTRUMENT, &CCtrlPianoRoll::OnInstrumentChanged)
	ON_CBN_SELENDOK(IDC_PIANOROLL_SNAPROWS, &CCtrlPianoRoll::OnViewOptionsChanged)
	ON_BN_CLICKED(IDC_PIANOROLL_SNAP, &CCtrlPianoRoll::OnViewOptionsChanged)
	ON_BN_CLICKED(IDC_PIANOROLL_ALLOWEDITING, &CCtrlPianoRoll::OnAllowEditingChanged)
	ON_BN_CLICKED(IDC_PIANOROLL_FOLLOWSONG, &CCtrlPianoRoll::OnViewOptionsChanged)
	ON_BN_CLICKED(IDC_PIANOROLL_UNDO, &CCtrlPianoRoll::OnUndo)
	ON_BN_CLICKED(IDC_PIANOROLL_REDO, &CCtrlPianoRoll::OnRedo)
	ON_BN_CLICKED(IDC_PIANOROLL_PLAY, &CCtrlPianoRoll::OnPlay)
	ON_BN_CLICKED(IDC_PIANOROLL_STOP, &CCtrlPianoRoll::OnStop)
	ON_BN_CLICKED(IDC_PIANOROLL_SPLITCHANNELS, &CCtrlPianoRoll::OnSplitChannels)
END_MESSAGE_MAP()

CCtrlPianoRoll::CCtrlPianoRoll(CModControlView &parent, CModDoc &document)
	: CModControlDlg(parent, document)
{
}

Setting<LONG> &CCtrlPianoRoll::GetSplitPosRef()
{
	return TrackerSettings::Instance().glPianoRollWindowHeight;
}

void CCtrlPianoRoll::DoDataExchange(CDataExchange *pDX)
{
	CModControlDlg::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PIANOROLL_PATTERN, m_pattern);
	DDX_Control(pDX, IDC_PIANOROLL_INSTRUMENT, m_instrument);
	DDX_Control(pDX, IDC_PIANOROLL_SNAPROWS, m_snapRows);
}

BOOL CCtrlPianoRoll::OnInitDialog()
{
	CModControlDlg::OnInitDialog();
	RebuildSelectors();
	m_snapRows.SetItemData(m_snapRows.AddString(_T("1")), 1);
	m_snapRows.SetItemData(m_snapRows.AddString(_T("2")), 2);
	m_snapRows.SetItemData(m_snapRows.AddString(_T("4")), 4);
	m_snapRows.SetItemData(m_snapRows.AddString(_T("8")), 8);
	m_snapRows.SetCurSel(0);
	CheckDlgButton(IDC_PIANOROLL_SNAP, BST_CHECKED);
	CheckDlgButton(IDC_PIANOROLL_FOLLOWSONG, BST_CHECKED);
	CheckDlgButton(IDC_PIANOROLL_ALLOWEDITING, BST_UNCHECKED);
	UpdateEditControls();
	m_initialized = true;
	return TRUE;
}

bool CCtrlPianoRoll::SelectComboData(CComboBox &combo, DWORD_PTR value)
{
	for(int index = 0; index < combo.GetCount(); ++index)
	{
		if(combo.GetItemData(index) == value)
		{
			combo.SetCurSel(index);
			return true;
		}
	}
	return false;
}

void CCtrlPianoRoll::RebuildSelectors()
{
	if(!m_pattern.m_hWnd)
		return;
	m_pattern.SetRedraw(FALSE);
	m_pattern.ResetContent();
	for(PATTERNINDEX pattern = 0; pattern < m_sndFile.Patterns.Size(); ++pattern)
	{
		if(!m_sndFile.Patterns.IsValidPat(pattern)) continue;
		CString label;
		label.Format(_T("%u"), pattern);
		const int index = m_pattern.AddString(label);
		m_pattern.SetItemData(index, pattern);
	}
	if(!SelectComboData(m_pattern, m_currentPattern) && m_pattern.GetCount() > 0)
	{
		m_pattern.SetCurSel(0);
		m_currentPattern = static_cast<PATTERNINDEX>(m_pattern.GetItemData(0));
	}
	m_pattern.SetRedraw(TRUE);

	m_instrument.ResetContent();
	const int none = m_instrument.AddString(_T("Choose instrument / sample"));
	m_instrument.SetItemData(none, 0);
	if(m_sndFile.GetNumInstruments() > 0)
	{
		for(INSTRUMENTINDEX instrument = 1; instrument <= m_sndFile.GetNumInstruments(); ++instrument)
		{
			if(m_sndFile.Instruments[instrument] == nullptr) continue;
			CString label = m_modDoc.GetPatternViewInstrumentName(instrument);
			const int index = m_instrument.AddString(label);
			m_instrument.SetItemData(index, instrument);
		}
	} else
	{
		for(SAMPLEINDEX sample = 1; sample <= m_sndFile.GetNumSamples(); ++sample)
		{
			if(!m_sndFile.GetSample(sample).HasSampleData() && !m_sndFile.GetSample(sample).uFlags[CHN_ADLIB]) continue;
			CString label;
			label.Format(_T("%u: %s"), sample, mpt::ToCString(m_sndFile.GetCharsetInternal(), m_sndFile.m_szNames[sample]).GetString());
			const int index = m_instrument.AddString(label);
			m_instrument.SetItemData(index, sample);
		}
	}
	if(m_selectedInstrument == 0 || !SelectComboData(m_instrument, m_selectedInstrument))
	{
		// A blank Piano Roll should be immediately writable. Prefer the first
		// real instrument / sample instead of leaving the non-value prompt active.
		const int initialSelection = m_instrument.GetCount() > 1 ? 1 : 0;
		m_instrument.SetCurSel(initialSelection);
		m_selectedInstrument = static_cast<ModCommand::INSTR>(m_instrument.GetItemData(initialSelection));
	}
}

ROWINDEX CCtrlPianoRoll::GetSnapRows() const
{
	const int selected = m_snapRows.GetCurSel();
	return selected >= 0 ? static_cast<ROWINDEX>(m_snapRows.GetItemData(selected)) : 1;
}

PianoRollPattern::ViewFilter CCtrlPianoRoll::GetViewFilter() const
{
	return {};
}

void CCtrlPianoRoll::SetCurrentPattern(PATTERNINDEX pattern)
{
	if(!m_sndFile.Patterns.IsValidPat(pattern)) return;
	m_currentPattern = pattern;
	SelectComboData(m_pattern, pattern);
	if(CChildFrame *frame = static_cast<CChildFrame *>(GetParentFrame()))
	{
		// This is the sole synchronization with the Tracker page.  Its cursor,
		// selection and viewport remain in PatternViewState.
		auto &tracker = frame->GetPatternViewState();
		tracker.nPattern = pattern;
		tracker.initialized = true;
	}
	SyncView();
}

void CCtrlPianoRoll::SyncView()
{
	if(m_hWndView)
		SendViewMessage(VIEWMSG_SETCURRENTPATTERN, m_currentPattern);
}

void CCtrlPianoRoll::OnPatternChanged()
{
	const int selected = m_pattern.GetCurSel();
	if(selected >= 0) SetCurrentPattern(static_cast<PATTERNINDEX>(m_pattern.GetItemData(selected)));
}

void CCtrlPianoRoll::OnInstrumentChanged()
{
	const int selected = m_instrument.GetCurSel();
	if(selected >= 0) m_selectedInstrument = static_cast<ModCommand::INSTR>(m_instrument.GetItemData(selected));
}

void CCtrlPianoRoll::OnViewOptionsChanged()
{
	if(m_hWndView)
	{
		SendViewMessage(VIEWMSG_FOLLOWSONG, GetFollowSong());
		SyncView();
	}
}

void CCtrlPianoRoll::OnAllowEditingChanged()
{
	m_allowEditing = false;
	if(IsDlgButtonChecked(IDC_PIANOROLL_ALLOWEDITING) == BST_CHECKED)
	{
		m_allowEditing = Reporting::Confirm(
			_T("Piano Roll editing is experimental and contains bugs.\n\n")
			_T("Editing requires each track (channel) to use only one instrument or sample.\n\n")
			_T("Allow editing?"), _T("Experimental Piano Roll Editing"), false, true, this) == cnfYes;
	}
	CheckDlgButton(IDC_PIANOROLL_ALLOWEDITING, m_allowEditing ? BST_CHECKED : BST_UNCHECKED);
	UpdateEditControls();
}

void CCtrlPianoRoll::UpdateEditControls()
{
	GetDlgItem(IDC_PIANOROLL_UNDO)->EnableWindow(m_modDoc.GetPatternUndo().CanUndo());
	GetDlgItem(IDC_PIANOROLL_REDO)->EnableWindow(m_modDoc.GetPatternUndo().CanRedo());
	GetDlgItem(IDC_PIANOROLL_SPLITCHANNELS)->EnableWindow(m_allowEditing);
}

void CCtrlPianoRoll::OnSplitChannels()
{
	if(m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, IDC_PIANOROLL_SPLITCHANNELS, 0);
}

void CCtrlPianoRoll::OnUndo() { if(m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_UNDO, 0); }
void CCtrlPianoRoll::OnRedo() { if(m_hWndView) ::SendMessage(m_hWndView, WM_COMMAND, ID_EDIT_REDO, 0); }
void CCtrlPianoRoll::OnPlay() { m_modDoc.OnPatternPlay(); SwitchToViewIfMouse(); }
void CCtrlPianoRoll::OnStop()
{
	if(CMainFrame *frame = CMainFrame::GetMainFrame()) frame->PauseMod(&m_modDoc);
	m_sndFile.ResetChannels();
	SwitchToViewIfMouse();
}

void CCtrlPianoRoll::RecalcLayout()
{
	// Dialog units and controls are DPI-aware through the normal dialog base.
}

void CCtrlPianoRoll::UpdateView(UpdateHint hint, CObject *)
{
	if(!m_initialized) return;
	if(hint.GetType()[HINT_MODTYPE] || hint.GetCategory() == HINTCAT_INSTRUMENTS || hint.GetCategory() == HINTCAT_SAMPLES || hint.GetCategory() == HINTCAT_GENERAL)
		RebuildSelectors();
	if(hint.GetType()[HINT_UNDO])
	{
		UpdateEditControls();
	}
}

CRuntimeClass *CCtrlPianoRoll::GetAssociatedViewClass() { return RUNTIME_CLASS(CViewPianoRoll); }

LRESULT CCtrlPianoRoll::OnModCtrlMsg(WPARAM wParam, LPARAM lParam)
{
	switch(wParam)
	{
	case CTRLMSG_GETCURRENTPATTERN: return m_currentPattern;
	case CTRLMSG_SETCURRENTINSTRUMENT:
	case CTRLMSG_PAT_SETINSTRUMENT:
		m_selectedInstrument = static_cast<ModCommand::INSTR>(lParam);
		SelectComboData(m_instrument, m_selectedInstrument);
		return 1;
	case CTRLMSG_SETVIEWWND:
		OnViewOptionsChanged();
		SyncView();
		break;
	default:
		return CModControlDlg::OnModCtrlMsg(wParam, lParam);
	}
	return 0;
}

void CCtrlPianoRoll::OnActivatePage(LPARAM lParam)
{
	if(CChildFrame *frame = static_cast<CChildFrame *>(GetParentFrame()))
	{
		auto &state = frame->GetPianoRollViewState();
		const int trackerInstrument = m_parent.GetInstrumentChange();
		if(state.initialized)
		{
			m_selectedInstrument = state.instrument != 0 ? state.instrument
				: static_cast<ModCommand::INSTR>(std::max(0, trackerInstrument));
			if((m_selectedInstrument == 0 || !SelectComboData(m_instrument, m_selectedInstrument)) && m_instrument.GetCount() > 1)
			{
				m_instrument.SetCurSel(1);
				m_selectedInstrument = static_cast<ModCommand::INSTR>(m_instrument.GetItemData(1));
			}
			CheckDlgButton(IDC_PIANOROLL_SNAP, state.snap ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(IDC_PIANOROLL_FOLLOWSONG, state.followSong ? BST_CHECKED : BST_UNCHECKED);
			SelectComboData(m_snapRows, state.snapRows);
			SetCurrentPattern(state.nPattern);
		} else if(lParam != -1 && m_sndFile.Patterns.IsValidPat(static_cast<PATTERNINDEX>(lParam & 0xFFFF)))
		{
			if(trackerInstrument > 0 && SelectComboData(m_instrument, trackerInstrument))
				m_selectedInstrument = static_cast<ModCommand::INSTR>(trackerInstrument);
			SetCurrentPattern(static_cast<PATTERNINDEX>(lParam & 0xFFFF));
		} else
		{
			SetCurrentPattern(frame->GetPatternViewState().nPattern);
		}
		if(m_hWndView)
			SendViewMessage(VIEWMSG_LOADSTATE, reinterpret_cast<LPARAM>(&state));
	}
	OnViewOptionsChanged();
}

void CCtrlPianoRoll::OnDeactivatePage()
{
	if(CChildFrame *frame = static_cast<CChildFrame *>(GetParentFrame()); frame && m_hWndView)
		SendViewMessage(VIEWMSG_SAVESTATE, reinterpret_cast<LPARAM>(&frame->GetPianoRollViewState()));
}

OPENMPT_NAMESPACE_END
