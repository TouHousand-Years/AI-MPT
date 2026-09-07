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
bool BlockCommand(UINT command);
}
OPENMPT_NAMESPACE_END
