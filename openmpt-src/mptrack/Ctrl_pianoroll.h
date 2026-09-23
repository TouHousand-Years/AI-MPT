/* Piano Roll upper control panel. */
#pragma once

#include "openmpt/all/BuildSettings.hpp"
#include "Globals.h"
#include "PianoRoll.h"
#include "PianoRollPattern.h"
#include "resource.h"

OPENMPT_NAMESPACE_BEGIN

class CCtrlPianoRoll final : public CModControlDlg
{
private:
	CComboBox m_pattern, m_instrument, m_snapRows;
	PATTERNINDEX m_currentPattern = 0;
	ModCommand::INSTR m_selectedInstrument = 0;
	bool m_allowEditing = false;

public:
	CCtrlPianoRoll(CModControlView &parent, CModDoc &document);

	PATTERNINDEX GetCurrentPattern() const { return m_currentPattern; }
	ModCommand::INSTR GetSelectedInstrument() const { return m_selectedInstrument; }
	ROWINDEX GetSnapRows() const;
	bool AllowEditing() const { return m_allowEditing; }
	bool SnapEnabled() const { return IsDlgButtonChecked(IDC_PIANOROLL_SNAP) != BST_UNCHECKED; }
	bool GetFollowSong() const { return IsDlgButtonChecked(IDC_PIANOROLL_FOLLOWSONG) != BST_UNCHECKED; }
	PianoRollPattern::ViewFilter GetViewFilter() const;
	void SetCurrentPattern(PATTERNINDEX pattern);

	Setting<LONG> &GetSplitPosRef() override;
	BOOL OnInitDialog() override;
	void DoDataExchange(CDataExchange *pDX) override;
	void RecalcLayout() override;
	void UpdateView(UpdateHint hint, CObject *pObject = nullptr) override;
	CRuntimeClass *GetAssociatedViewClass() override;
	LRESULT OnModCtrlMsg(WPARAM wParam, LPARAM lParam) override;
	void OnActivatePage(LPARAM lParam) override;
	void OnDeactivatePage() override;

private:
	void RebuildSelectors();
	bool SelectComboData(CComboBox &combo, DWORD_PTR value);
	void SyncView();
	void OnPatternChanged();
	void OnInstrumentChanged();
	void OnViewOptionsChanged();
	void OnAllowEditingChanged();
	void UpdateEditControls();
	void OnSplitChannels();
	void OnUndo();
	void OnRedo();
	void OnPlay();
	void OnStop();
	DECLARE_MESSAGE_MAP()
};

OPENMPT_NAMESPACE_END
