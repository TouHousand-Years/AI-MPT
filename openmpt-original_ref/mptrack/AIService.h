#pragma once
#include "openmpt/all/BuildSettings.hpp"
OPENMPT_NAMESPACE_BEGIN
class CModDoc;
namespace AI
{
// Stable tab item ID for the embedded AI page. It is not a dialog ID; the tab
// is resolved by item data so modules without an Instruments tab still map
// their physical tab index to CModControlView::Page::AI correctly.
constexpr UINT PanelPageId = 49001;
void Start(CWnd &owner);
void Stop();
// Issue 35: the panel is a hidden WS_CHILD of the main frame until the active
// CModControlView attaches it inside its tab client area.
bool AttachPanel(CWnd &host, const CRect &rect);
// Moves the panel only if it is currently attached to host; a stale view must
// not reposition a panel that another document's view has claimed.
void LayoutPanel(CWnd &host, const CRect &rect);
// Hides the panel and parks it back on the main frame. When host is given, the
// call is ignored unless the panel is currently attached to that host.
void DetachPanel(CWnd *host = nullptr);
void DocumentClosed(CModDoc &document);
bool FilterInput(MSG &message);
bool IsReadOnlyCommand(UINT command);
bool BlockCommand(UINT command);
// Appends a timestamped line to the integration trace file (<report>.trace);
// a no-op unless OPENMPT_AI_ENDPOINT_REPORT is set.
void TestTrace(const std::string &text);
// Called once per audio buffer from the realtime callback (issue 30). Traces a
// violation when the calling thread is inside an AI IPC read/write/wait path,
// plus one first-call line so tests can prove the check ran on the audio thread.
void AudioCallbackIpcCheck();
#ifdef ENABLE_TESTS
void IntegrationHost(CWnd &owner, CModDoc &document, const wchar_t *report);
#endif
}
OPENMPT_NAMESPACE_END
