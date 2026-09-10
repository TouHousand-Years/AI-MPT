/* Stable integration identifiers for the document-owned Piano Roll page. */
#pragma once

#include "openmpt/all/BuildSettings.hpp"

OPENMPT_NAMESPACE_BEGIN
namespace PianoRoll
{
// Tab item data, deliberately independent of dialog resource IDs and of the
// physical tab index (which varies for modules without Instruments).
constexpr UINT PanelPageId = 49002;
}
OPENMPT_NAMESPACE_END
