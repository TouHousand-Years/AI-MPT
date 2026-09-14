#pragma once
#include "openmpt/all/BuildSettings.hpp"
#include "mpt/json/json.hpp"
#include "../soundlib/modcommand.h"
#include "PatternCursor.h"
#include <chrono>
#include <optional>

OPENMPT_NAMESPACE_BEGIN
class CModDoc;
namespace AI
{
using Json = nlohmann::json;
Json Failure(const char *code, const char *reason, const char *layer = "capability");

// All model access, including construction and destruction, belongs to the UI thread.
// A pending expansion or Pattern switch returns no wire reply until its resolver resumes the call.
class PatternCapability
{
public:
	using Clock = std::chrono::steady_clock;
	PatternCapability(CModDoc &document, PATTERNINDEX pattern, std::optional<PatternRect> selection);
	~PatternCapability();
	PatternCapability(const PatternCapability &) = delete;
	PatternCapability &operator=(const PatternCapability &) = delete;
	Json Call(const std::string &tool, const Json &arguments);
	Json Apply(bool whole = true, bool simulateFailure = false);
	Json Reject();
	Json Review() const;
	Json ExpansionRange() const;
	Json ResolveExpansion(bool approve);
	// Approval metadata for the review UI while PendingSwitch() is true.
	Json SwitchRange() const;
	Json ResolveSwitch(bool approve);
	void ForceRelease();
	void Tick(Clock::time_point now = Clock::now());
	bool Occupied() const { return m_retained; }
	bool PendingExpansion() const { return m_pending.has_value(); }
	bool PendingSwitch() const { return m_pendingSwitch.has_value(); }
	bool HasProposal() const { return m_proposal; }
	PATTERNINDEX Pattern() const { return m_pattern; }
	// Returns the resolution of any pending work that the new preference turns on.
	Json Configure(unsigned seconds, bool rangeAlways, bool switchAlways = false, bool autoAccept = false);
private:
	struct CaptureState
	{
		PATTERNINDEX pattern = PATTERNINDEX_INVALID;
		ROWINDEX rows = 0;
		CHANNELINDEX channels = 0;
		PatternRect envelope;
		std::vector<ModCommand> baseline, candidate;
		std::string signature, token;
	};
	struct SwitchRequest
	{
		PATTERNINDEX source = PATTERNINDEX_INVALID;
		PATTERNINDEX target = PATTERNINDEX_INVALID;
		bool create = false;
		std::string sourceSignature;
		std::string token;
		bool authenticated = false;
	};
	CModDoc &m_doc;
	const DWORD m_thread;
	PATTERNINDEX m_pattern;
	std::optional<PatternRect> m_selection;
	PatternRect m_envelope;
	std::vector<ModCommand> m_baseline, m_candidate;
	std::string m_signature, m_token;
	std::optional<Json> m_pending;
	std::optional<SwitchRequest> m_pendingSwitch;
	ROWINDEX m_rows = 0;
	CHANNELINDEX m_channels = 0;
	unsigned m_timeout = 300;
	bool m_retained = false, m_proposal = false, m_alwaysApprove = false, m_alwaysSwitch = false, m_alwaysAccept = false;
	bool m_ownsOccupancy = false, m_callActive = false;
	Clock::time_point m_deadline;
	std::string Signature() const;
	std::string Signature(PATTERNINDEX pattern) const;
	Json ContextFor(PATTERNINDEX pattern, ROWINDEX rows, CHANNELINDEX channels, const std::vector<ModCommand> &cells, const Json &args) const;
	Json Context(const std::vector<ModCommand> &cells, const Json &args) const;
	Json PatternOrder() const;
	Json ReorderPatternOrder(const Json &args);
	Json PatternInfo(PATTERNINDEX pattern) const;
	Json OrderReferences(PATTERNINDEX pattern) const;
	CaptureState CapturePattern(PATTERNINDEX pattern, std::optional<PatternRect> selection) const;
	void Adopt(CaptureState state);
	Json Replace(const Json &args, bool approved = false);
	Json Switch(const Json &args);
	Json PerformSwitch(const SwitchRequest &request);
	Json Diff(const std::vector<ModCommand> &before, const std::vector<ModCommand> &after) const;
	Json Validate(const ModCommand &before, const ModCommand &after, ROWINDEX row) const;
	void Capture();
	Json Dispatch(const std::string &tool, const Json &arguments);
};
}
OPENMPT_NAMESPACE_END
