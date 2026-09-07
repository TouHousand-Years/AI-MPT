#pragma once
#include "openmpt/all/BuildSettings.hpp"
OPENMPT_NAMESPACE_BEGIN
class CModDoc;
namespace AI
{
constexpr UINT ShowPanelCommand = 49000;
void Start(CWnd &owner);
void Stop();
void ShowPanel();
void DocumentClosed(CModDoc &document);
bool FilterInput(MSG &message);
bool IsReadOnlyCommand(UINT command);
bool BlockCommand(UINT command);
#ifdef ENABLE_TESTS
void IntegrationHost(CWnd &owner, CModDoc &document, const wchar_t *report);
#endif
}
OPENMPT_NAMESPACE_END
