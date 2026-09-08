#include "stdafx.h"
#ifdef ENABLE_TESTS
#include "../mptrack/AIPattern.h"
#include "../mptrack/Mptrack.h"
#include "../mptrack/Moddoc.h"
#include <fstream>
#include <stdexcept>
#include <thread>

OPENMPT_NAMESPACE_BEGIN
namespace Test
{
static void Require(bool condition, const char *message) { if(!condition) throw std::runtime_error(message); }
static bool OK(const AI::Json &r) { return r.at("ok").get<bool>(); }
static std::string ErrorCode(const AI::Json &result) { return result.at("error").at("code").get<std::string>(); }
static AI::Json Cell(int note, int instrument = 2, int volume = 40)
{
	return {{"note", note}, {"instrument", instrument}, {"volume_command", 1}, {"volume", volume}, {"effect_command", 0}, {"effect_parameter", 0}};
}
static AI::Json Raw(const ModCommand &cell)
{
	return {{"note", cell.note}, {"instrument", cell.instr}, {"volume_command", cell.volcmd}, {"volume", cell.vol},
		{"effect_command", cell.command}, {"effect_parameter", cell.param}};
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
		AI::PatternCapability owner(*doc, 0, {});
		const auto read = owner.Call("get_pattern_context", AI::Json::object());
		Require(OK(read) && !doc->AIOccupied() && !read.contains("session"), "Short read releases occupancy");
		Require(read.contains("ending_reminder"), "Every tool result explains session endings");
		const auto token = owner.Call("get_pattern_context", {{"occupy", true}}).at("session");
		{
			AI::PatternCapability other(*doc, 0, {});
			Require(other.Call("get_pattern_context", AI::Json::object())["error"]["code"] == "busy", "Other facade cannot acquire occupied document");
		}
		Require(doc->AIOccupied(), "Destroying another facade must not release the owner's occupancy");
		Require(OK(owner.Call("abort_session", {{"session", token}})), "Owner can end occupancy");
	}
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
		AI::PatternCapability cap(*doc, 0, PatternRect(PatternCursor(0, 1), PatternCursor(7, 1, PatternCursor::lastColumn)));
		const auto read = cap.Call("get_pattern_context", {{"occupy", true}});
		const auto token = read.at("session");
		const auto context = read.at("context");
		Require(context["pattern"] == 0 && context["rows"] == 128 && context["channels"] == 4, "Fixture identity and dimensions");
		Require(context["timing"]["default_tempo"] == 125 && context["timing"]["default_speed"] == 6
			&& context["timing"]["rows_per_beat"] == 4 && context["timing"]["rows_per_measure"] == 16, "Fixture timing");
		Require(context["format"]["name"] == "mptm" && context["format"]["volume_max"] == 64, "Format write limits");
		Require(context["instruments"].size() == 3 && context["samples"].size() == 3, "Existing resources summarized");
		Require(context["cells"].size() > 0 && context["cells"][0]["note_kind"] == "pitched", "Sparse semantic cells");
		const AI::Json range{{"first_row", 0}, {"row_count", 8}, {"first_channel", 0}, {"channel_count", 1}};
		const auto narrowed = cap.Call("get_pattern_context", {{"session", token}, {"range", range}});
		Require(OK(narrowed) && narrowed["context"]["range"] == range && !narrowed["context"]["cells"].empty(), "Reads may narrow outside write envelope");
		for(const auto &entry : narrowed["context"]["cells"])
			Require(entry["row"] < 8 && entry["channel"] == 0 && entry["raw"].size() == 6, "Exact sparse range with complete raw cells");
		Require(!OK(cap.Call("get_pattern_context", {{"session", token}, {"pattern", 1}})), "Read cannot select another Pattern");
		Require(doc->AIOccupied(), "Omitting occupy never demotes retained occupancy");
		cap.ForceRelease();
	}
	{
		auto &sf = doc->GetSoundFile();
		const auto original = *sf.Patterns[0].GetpModCommand(0, 1);
		const size_t patternCellCount = size_t(sf.Patterns[0].GetNumRows()) * sf.GetNumChannels();
		const std::vector<ModCommand> documentBaseline(sf.Patterns[0].GetpModCommand(0, 0), sf.Patterns[0].GetpModCommand(0, 0) + patternCellCount);
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(doc->AIOccupied(), "Retained read locks the document");
		Require(doc->GetPatternUndo().Undo() == PATTERNINDEX_INVALID, "Native Undo is blocked during retained occupancy");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 1, 49))), "First voice");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 2, 53))), "Second voice");
		const auto beforeFailure = cap.Call("get_pattern_context", {{"session", token}}).at("context");
		auto invalid = Segment(token, 1, 60);
		invalid["cells"].push_back({{"row", 1}, {"cell", Cell(62, 2, 65)}});
		auto failure = cap.Call("replace_pattern_segment", invalid);
		Require(!OK(failure) && failure["error"]["row"] == 1 && failure["error"]["field"] == "volume"
			&& failure["error"]["value"] == 65 && failure["error"]["reason"].is_string(), "Typed invalid volume");
		Require(cap.Call("get_pattern_context", {{"session", token}}).at("context") == beforeFailure, "Failed call leaves candidate unchanged");
		Require(!OK(cap.Call("release_occupancy", {{"session", token}})), "Edited candidate cannot be released");
		const auto revisionBeforeHandoff = doc->AIRevision();
		const auto handoff = cap.Call("handoff_for_review", {{"session", token}});
		Require(OK(handoff) && !doc->AIOccupied(), "Handoff freezes and releases");
		const auto proposal = cap.Review();
		Require(proposal["status"] == "current" && handoff["diff"].size() == 2 && proposal["diff"] == handoff["diff"], "Multi-voice calls normalize to one current whole-proposal diff");
		for(const auto &change : proposal["diff"])
		{
			const auto &live = *sf.Patterns[0].GetpModCommand(change["row"].get<ROWINDEX>(), change["channel"].get<CHANNELINDEX>());
			Require(Raw(live) == change["before"], "Before Apply the current document equals every baseline cell");
		}
		Require(doc->AIRevision() == revisionBeforeHandoff, "Review handoff does not change the document revision");
		Require(std::equal(documentBaseline.begin(), documentBaseline.end(), sf.Patterns[0].GetpModCommand(0, 0)), "Before Apply the complete current Pattern equals the baseline");
		if(const wchar_t *demo = _wgetenv(L"OPENMPT_AI_DEMO_REPORT"))
			std::ofstream(demo) << AI::Json{{"accumulated_candidate_diff", handoff["diff"]}, {"final_proposal", proposal}}.dump(2) << '\n';
		const auto revision = doc->AIRevision();
		const auto undoName = doc->GetPatternUndo().GetUndoName();
		const auto immutableProposal = cap.Review();
		const auto partial = cap.Apply(false);
		Require(!OK(partial) && ErrorCode(partial) == "unsupported", "Partial acceptance unsupported");
		Require(cap.HasProposal() && cap.Review() == immutableProposal, "Unsupported partial acceptance preserves the immutable proposal");
		const auto commitFailure = cap.Apply(true, true);
		Require(!OK(commitFailure) && ErrorCode(commitFailure) == "commitFailed", "Simulated commit failure rejected");
		Require(doc->AIRevision() == revision && doc->GetPatternUndo().GetUndoName() == undoName, "Failed Apply leaves revision and Undo untouched");
		Require(std::equal(documentBaseline.begin(), documentBaseline.end(), sf.Patterns[0].GetpModCommand(0, 0)), "Failed Apply leaves every Pattern cell untouched");
		Require(cap.HasProposal() && cap.Review() == immutableProposal, "Failed Apply preserves the same immutable proposal for retry");
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
		const auto undoBeforeReject = doc->GetPatternUndo().GetUndoName();
		const std::vector<ModCommand> patternBeforeReject(sf.Patterns[0].GetpModCommand(0, 0), sf.Patterns[0].GetpModCommand(0, 0) + patternCellCount);
		Require(OK(cap.Reject()) && doc->AIRevision() == revisionBeforeReject && doc->GetPatternUndo().CanRedo()
			&& doc->GetPatternUndo().GetUndoName() == undoBeforeReject, "Reject preserves revision and Undo/Redo history");
		Require(std::equal(patternBeforeReject.begin(), patternBeforeReject.end(), sf.Patterns[0].GetpModCommand(0, 0)), "Reject leaves every Pattern cell unchanged");
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(doc->GetPatternUndo().Redo() == PATTERNINDEX_INVALID && doc->GetPatternUndo().CanRedo(), "Native Redo is blocked during retained occupancy");
		cap.Call("replace_pattern_segment", Segment(token, 1, 50));
		cap.Call("handoff_for_review", {{"session", token}});
		const auto currentProposal = cap.Review();
		const auto saved = *sf.Patterns[0].GetpModCommand(0, 0);
		sf.Patterns[0].GetpModCommand(0, 0)->note++;
		Require(ErrorCode(cap.Apply()) == "stale" && cap.HasProposal(), "Stale proposal refused and retained");
		const auto staleProposal = cap.Review();
		Require(staleProposal["status"] == "stale" && staleProposal["diff"] == currentProposal["diff"], "Stale proposal remains immutable and available read-only without rebasing");
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
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 1, 55))), "Candidate exists before human release");
		cap.ForceRelease();
		Require(!doc->AIOccupied() && !OK(cap.Call("get_pattern_context", {{"session", token}})), "Human release invalidates token");
		const AI::Json releasedRange{{"first_row", 0}, {"row_count", 1}, {"first_channel", 1}, {"channel_count", 1}};
		const auto afterRelease = cap.Call("get_pattern_context", {{"range", releasedRange}});
		Require(afterRelease["context"]["cells"].empty(), "Human release discards the candidate");
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
		auto &sf = doc->GetSoundFile();
		const auto live = *sf.Patterns[0].GetpModCommand(0, 1);
		const auto revision = doc->AIRevision();
		const auto undoName = doc->GetPatternUndo().GetUndoName();
		AI::PatternCapability cap(*doc, 0, PatternRect(PatternCursor(0, 0), PatternCursor(7, 0, PatternCursor::lastColumn)));
		const auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(cap.Call("replace_pattern_segment", Segment(token, 1, 49)).value("pending_approval", false) && cap.PendingExpansion(), "Expansion approval is pending before human release");
		cap.ForceRelease();
		Require(!cap.Occupied() && !cap.PendingExpansion() && !doc->AIOccupied(), "Human release clears retained occupancy and the pending approval");
		Require(ErrorCode(cap.ResolveExpansion(true)) == "occupancyLost"
			&& ErrorCode(cap.Call("get_pattern_context", {{"session", token}})) == "occupancyLost", "Human release invalidates the approval and session token");
		Require(*sf.Patterns[0].GetpModCommand(0, 1) == live && doc->AIRevision() == revision
			&& doc->GetPatternUndo().GetUndoName() == undoName, "Human release during approval changes no document or Undo state");
	}
	{
		AI::PatternCapability cap(*doc, 0, PatternRect(PatternCursor(0, 0), PatternCursor(7, 0, PatternCursor::lastColumn)));
		cap.Configure(300, false);
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(cap.Call("replace_pattern_segment", Segment(token, 1, 49)).value("pending_approval", false), "Ask-each-time preference prompts for expansion");
		Require(ErrorCode(cap.ResolveExpansion(false)) == "rangeRejected", "Owner can decline the prompted expansion");
		cap.Configure(300, true);
		const auto expanded = cap.Call("replace_pattern_segment", Segment(token, 1, 49));
		Require(OK(expanded) && !expanded.contains("pending_approval"), "Changing to always-approve affects the subsequent expansion request");
		cap.Call("abort_session", {{"session", token}});
	}
	{
		auto &cell = *doc->GetSoundFile().Patterns[0].GetpModCommand(0, 1);
		const auto saved = cell; cell.command = CMD_TEMPO; cell.param = 125;
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		auto besideEffect = Segment(token, 1, 49);
		besideEffect["cells"][0]["cell"]["effect_command"] = CMD_TEMPO;
		besideEffect["cells"][0]["cell"]["effect_parameter"] = 125;
		Require(OK(cap.Call("replace_pattern_segment", besideEffect)), "A pitched note may change beside a byte-preserved effect");
		cap.ForceRelease(); cell = saved;
	}
	{
		auto &cell = *doc->GetSoundFile().Patterns[0].GetpModCommand(0, 1);
		const auto saved = cell; cell.param = 125;
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(!OK(cap.Call("replace_pattern_segment", Segment(token, 1, 49))), "Raw effect parameter must be preserved even when its command is empty");
		cap.ForceRelease(); cell = saved;
	}
	{
		auto &cell = *doc->GetSoundFile().Patterns[0].GetpModCommand(0, 1);
		const auto saved = cell; cell.note = NOTE_PC; cell.command = CMD_TEMPO; cell.param = 123;
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(!OK(cap.Call("replace_pattern_segment", Segment(token, 1, 49))), "Unsupported PC content cannot be clobbered");
		cap.ForceRelease(); cell = saved;
	}
	{
		auto &cell = *doc->GetSoundFile().Patterns[0].GetpModCommand(0, 1);
		const auto saved = cell; cell.note = NOTE_NOTECUT; cell.instr = 2;
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		auto changedSpecial = Segment(token, 1, NOTE_NOTECUT);
		changedSpecial["cells"][0]["cell"]["instrument"] = 1;
		Require(!OK(cap.Call("replace_pattern_segment", changedSpecial)), "Unsupported special-note cell must remain byte-for-byte identical");
		cap.ForceRelease(); cell = saved;
	}
	doc->SetModified(false);
	doc->OnCloseDocument();
}
}
OPENMPT_NAMESPACE_END
#endif
