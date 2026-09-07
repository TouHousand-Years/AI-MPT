#include "stdafx.h"
#ifdef ENABLE_TESTS
#include "../mptrack/AIPattern.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/Moddoc.h"
#include <stdexcept>
#include <thread>

OPENMPT_NAMESPACE_BEGIN
namespace Test
{
static void Require(bool condition, const char *message) { if(!condition) throw std::runtime_error(message); }
static bool OK(const AI::Json &r) { return r.at("ok").get<bool>(); }
static AI::Json Cell(int note, int instrument = 2, int volume = 40)
{
	return {{"note", note}, {"instrument", instrument}, {"volume_command", 1}, {"volume", volume}, {"effect_command", 0}, {"effect_parameter", 0}};
}
static AI::Json Segment(const AI::Json &token, int channel, int note)
{
	return {{"session", token}, {"channel", channel}, {"first_row", 0}, {"row_count", 8},
		{"cells", AI::Json::array({{{"row", 0}, {"cell", Cell(note)}}})}};
}
void AIPatternTests(const CString &fixture)
{
	auto *doc = static_cast<CModDoc *>(theApp.OpenDocumentFile(fixture, FALSE));
	if(!doc) throw std::runtime_error("Cannot load collaboration fixture");
	{
		AI::PatternCapability capability(*doc, 0, {});
		const auto result = capability.Call("get_pattern_context", {{"occupy", true}});
		if(!result.at("ok").get<bool>() || result.at("context").at("rows") != 128)
			throw std::runtime_error("Context must describe the fixture's real 128-row Pattern");
		if(!capability.Call("release_occupancy", {{"session", result.at("session")}}).at("ok").get<bool>())
			throw std::runtime_error("Read-only retained occupancy must be releasable");
		const auto occupied = capability.Call("get_pattern_context", {{"occupy", true}});
		const auto token = occupied.at("session");
		const auto segment = AI::Json{{"session", token}, {"channel", 1}, {"first_row", 0}, {"row_count", 8},
			{"cells", AI::Json::array({{{"row", 0}, {"cell", {{"note", 49}, {"instrument", 2}, {"volume_command", 1}, {"volume", 40}, {"effect_command", 0}, {"effect_parameter", 0}}}}})}};
		if(!capability.Call("replace_pattern_segment", segment).at("ok").get<bool>())
			throw std::runtime_error("A legal harmony segment must update the private candidate");
		if(doc->GetSoundFile().Patterns[0].GetpModCommand(0, 1)->note != NOTE_NONE)
			throw std::runtime_error("Private candidate must never mutate the live Pattern");
		capability.Call("abort_session", {{"session", token}});
	}
	{
		auto &sf = doc->GetSoundFile();
		const auto original = *sf.Patterns[0].GetpModCommand(0, 1);
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(doc->AIOccupied(), "Retained read locks the document");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 1, 49))), "First voice");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 2, 53))), "Second voice");
		const auto beforeFailure = cap.Call("get_pattern_context", {{"session", token}}).at("context");
		auto invalid = Segment(token, 1, 60); invalid["cells"][0]["cell"]["volume"] = 65;
		auto failure = cap.Call("replace_pattern_segment", invalid);
		Require(!OK(failure) && failure["error"]["row"] == 0 && failure["error"]["field"] == "volume", "Typed invalid volume");
		Require(cap.Call("get_pattern_context", {{"session", token}}).at("context") == beforeFailure, "Failed call leaves candidate unchanged");
		Require(!OK(cap.Call("release_occupancy", {{"session", token}})), "Edited candidate cannot be released");
		Require(OK(cap.Call("handoff_for_review", {{"session", token}})) && !doc->AIOccupied(), "Handoff freezes and releases");
		Require(cap.Review()["diff"].size() == 2, "Multi-voice calls normalize to two final cells");
		const auto revision = doc->AIRevision();
		const auto undoName = doc->GetPatternUndo().GetUndoName();
		Require(!OK(cap.Apply(false)), "Partial acceptance unsupported");
		Require(!OK(cap.Apply(true, true)), "Simulated failure rejected");
		Require(doc->AIRevision() == revision && doc->GetPatternUndo().GetUndoName() == undoName, "Failed Apply leaves revision and Undo untouched");
		Require(*sf.Patterns[0].GetpModCommand(0, 1) == original, "Failed Apply leaves Pattern untouched");
		Require(OK(cap.Apply()), "Whole proposal applies");
		Require(doc->AIRevision() == revision + 1, "Apply increments revision exactly once");
		Require(sf.Patterns[0].GetpModCommand(0, 1)->note == 49 && sf.Patterns[0].GetpModCommand(0, 2)->note == 53, "Both voices committed");
		doc->GetPatternUndo().Undo();
		Require(*sf.Patterns[0].GetpModCommand(0, 1) == original && sf.Patterns[0].GetpModCommand(0, 2)->note == NOTE_NONE, "One native Undo restores both voices");
		Require(!doc->GetPatternUndo().CanUndo(), "Apply created only one native Undo step");
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		cap.Call("replace_pattern_segment", Segment(token, 1, 50));
		cap.Call("handoff_for_review", {{"session", token}});
		const auto revisionBeforeReject = doc->AIRevision();
		Require(OK(cap.Reject()) && doc->AIRevision() == revisionBeforeReject && doc->GetPatternUndo().CanRedo(), "Reject preserves revision and redo history");
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		cap.Call("replace_pattern_segment", Segment(token, 1, 50));
		cap.Call("handoff_for_review", {{"session", token}});
		const auto saved = *sf.Patterns[0].GetpModCommand(0, 0);
		sf.Patterns[0].GetpModCommand(0, 0)->note++;
		Require(cap.Apply()["error"]["code"] == "stale" && cap.HasProposal(), "Stale proposal refused and retained");
		*sf.Patterns[0].GetpModCommand(0, 0) = saved; cap.Reject();
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		auto other = *sf.Patterns[1].GetpModCommand(0, 0);
		sf.Patterns[1].GetpModCommand(0, 0)->note = 64;
		Require(OK(cap.Call("get_pattern_context", {{"session", token}})), "Unrelated Pattern edit does not stale session");
		*sf.Patterns[1].GetpModCommand(0, 0) = other;
		const auto volume = sf.GetSample(1).nVolume; sf.GetSample(1).nVolume--;
		Require(cap.Call("get_pattern_context", {{"session", token}})["error"]["code"] == "stale", "Sample dependency invalidates session");
		sf.GetSample(1).nVolume = volume;
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		cap.Tick(AI::PatternCapability::Clock::now() + std::chrono::seconds(301));
		Require(cap.Call("get_pattern_context", {{"session", token}})["error"]["code"] == "occupancyLost", "Timeout invalidates token");
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session"); cap.ForceRelease();
		Require(!doc->AIOccupied() && !OK(cap.Call("get_pattern_context", {{"session", token}})), "Human release invalidates token");
		AI::Json wrongThread;
		std::thread worker([&] { wrongThread = cap.Call("get_pattern_context", AI::Json::object()); }); worker.join();
		Require(wrongThread["error"]["code"] == "owningThreadRequired", "Worker thread cannot access model");
	}
	{
		AI::PatternCapability cap(*doc, 0, PatternRect(PatternCursor(0, 0), PatternCursor(7, 0, PatternCursor::lastColumn)));
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(cap.Call("replace_pattern_segment", Segment(token, 1, 49)).value("pending_approval", false), "Expansion waits for owner");
		cap.Tick(AI::PatternCapability::Clock::now() + std::chrono::hours(1));
		Require(cap.Occupied(), "Approval wait pauses occupancy timeout");
		Require(cap.ResolveExpansion(false)["error"]["code"] == "rangeRejected", "Declined expansion rejected");
		Require(cap.Call("get_pattern_context", {{"session", token}, {"baseline", true}})["context"] == cap.Call("get_pattern_context", {{"session", token}})["context"], "Decline preserves candidate");
		cap.Call("replace_pattern_segment", Segment(token, 1, 49));
		Require(OK(cap.ResolveExpansion(true)), "Approved expansion resumes request");
		Require(!cap.Call("replace_pattern_segment", Segment(token, 1, 50)).contains("pending_approval"), "Granted expansion is session-local and reused");
		cap.Call("abort_session", {{"session", token}});
	}
	{
		auto &cell = *doc->GetSoundFile().Patterns[0].GetpModCommand(0, 1);
		const auto saved = cell; cell.note = NOTE_PC; cell.command = CMD_TEMPO; cell.param = 123;
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(!OK(cap.Call("replace_pattern_segment", Segment(token, 1, 49))), "Unsupported PC content cannot be clobbered");
		cap.ForceRelease(); cell = saved;
	}
	doc->SetModified(false);
	doc->OnCloseDocument();
}
}
OPENMPT_NAMESPACE_END
#endif
