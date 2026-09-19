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
		auto &sf = doc->GetSoundFile();
		const auto savedOrder = sf.Order();
		sf.Order().assign(5, 0);
		sf.Order()[1] = PATTERNINDEX_SKIP;
		sf.Order()[3] = PATTERNINDEX_INVALID;
		sf.Order()[4] = 999;
		AI::PatternCapability cap(*doc, 1, {});
		const auto order = cap.Call("get_pattern_order", AI::Json::object());
		Require(OK(order), "Pattern order can be read without starting a session");
		Require(!doc->AIOccupied() && !order.contains("session"), "Order inspection never acquires write occupancy");
		Require(order["sequence"]["index"] == 0 && order["entries"].size() == 5, "Order reports current sequence and every entry");
		Require(order["entries"][0]["pattern"] == 0 && order["entries"][2]["pattern"] == 0
			&& order["entries"][2]["order"] == 2, "Repeated references preserve their order indices");
		Require(order["entries"][0]["rows"] == 128 && order["entries"][0]["name"].is_string(), "Valid entries carry Pattern metadata");
		Require(order["entries"][1]["kind"] == "skip" && order["entries"][3]["kind"] == "stop"
			&& order["entries"][4]["kind"] == "invalid", "Skip, stop and invalid references remain distinguishable");
		Require(order["unreferenced_patterns"].size() == 1 && order["unreferenced_patterns"][0]["pattern"] == 1,
			"Valid unreferenced Patterns remain discoverable");
		const auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(OK(cap.Call("get_pattern_order", AI::Json::object())) && doc->AIOccupied(), "Order inspection preserves an existing session");
		const auto invalidReorder = cap.Call("reorder_pattern_order", {{"session", token}, {"order", AI::Json::array({0, 0, 2, 3, 4})}});
		Require(ErrorCode(invalidReorder) == "validationFailure" && sf.Order()[1] == PATTERNINDEX_SKIP,
			"Order reordering rejects a non-permutation without changing Sequence data");
		const auto reordered = cap.Call("reorder_pattern_order", {{"session", token}, {"order", AI::Json::array({4, 2, 0, 1, 3})}});
		Require(OK(reordered) && reordered["status"] == "reordered" && reordered["session"] == token,
			"Order reordering keeps the retained session");
		Require(reordered["entries"][0]["kind"] == "invalid" && reordered["entries"][1]["pattern"] == 0
			&& reordered["entries"][3]["kind"] == "skip" && reordered["entries"][4]["kind"] == "stop",
			"Order reordering moves every original entry, including markers");
		AI::Json duplicateInsertion = AI::Json::array({0, 1, 2, 3, 4});
		duplicateInsertion.push_back({{"pattern", 0}});
		const auto rejectedInsertion = cap.Call("reorder_pattern_order", {{"session", token}, {"order", duplicateInsertion}});
		Require(ErrorCode(rejectedInsertion) == "validationFailure" && sf.Order().size() == 5,
			"Order insertion accepts only Patterns absent from the current Sequence");
		AI::Json insertion = AI::Json::array({0});
		insertion.push_back({{"pattern", 1}});
		for(int sourceOrder = 1; sourceOrder < 5; ++sourceOrder) insertion.push_back(sourceOrder);
		const auto inserted = cap.Call("reorder_pattern_order", {{"session", token}, {"order", insertion}});
		Require(OK(inserted) && inserted["status"] == "reordered" && inserted["session"] == token
			&& inserted["entries"].size() == 6 && inserted["entries"][1]["pattern"] == 1,
			"An edited bound Pattern absent from Order can be inserted at a chosen destination");
		Require(inserted["inserted_patterns"].size() == 1 && inserted["inserted_patterns"][0]["pattern"] == 1
			&& inserted["inserted_patterns"][0]["order"] == 1 && inserted["unreferenced_patterns"].empty(),
			"Order insertion reports the new reference and removes it from the unreferenced list");
		Require(cap.Call("reorder_pattern_order", {{"session", token}, {"order", AI::Json::array({0, 1, 2, 3, 4, 5})}})["status"] == "unchanged",
			"Identity Order permutation is an authenticated no-op");
		cap.Call("abort_session", {{"session", token}});
		const auto sequence = sf.Order.AddSequence();
		Require(sequence != SEQUENCEINDEX_INVALID, "Fixture supports another sequence");
		sf.Order.SetSequence(sequence);
		sf.Order().assign(1, 1);
		const auto changed = cap.Call("get_pattern_order", AI::Json::object());
		Require(OK(changed) && changed["sequence"]["index"] == sequence && changed["entries"].size() == 1
			&& changed["entries"][0]["pattern"] == 1, "Order inspection follows the current sequence");
		sf.Order.SetSequence(0);
		sf.Order.RemoveSequence(sequence);
		sf.Order() = savedOrder;
	}
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
		Require(context["format"]["volume_commands"].is_array() && context["format"]["volume_commands"].size() > 2
			&& context["format"]["effect_commands"].is_array() && context["format"]["effect_commands"].size() > 2,
			"Format publishes editable volume and effect command catalogs");
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
		AI::PatternCapability cap(*doc, 0, {});
		auto read = cap.Call("get_pattern_context", {{"occupy", true}});
		const auto token = read.at("session");
		for(const auto &command : read["context"]["format"]["volume_commands"])
		{
			auto segment = Segment(token, 1, 49);
			segment["cells"][0]["cell"]["volume_command"] = command["id"];
			segment["cells"][0]["cell"]["volume"] = command["parameter_max"];
			Require(OK(cap.Call("replace_pattern_segment", segment)), "Every advertised volume command and maximum parameter is writable");
		}
		for(const auto &command : read["context"]["format"]["effect_commands"])
		{
			auto segment = Segment(token, 1, 49);
			segment["cells"][0]["cell"]["effect_command"] = command["id"];
			segment["cells"][0]["cell"]["effect_parameter"] = command["parameter_max"];
			Require(OK(cap.Call("replace_pattern_segment", segment)), "Every advertised effect command and maximum parameter is writable");
		}
		auto invalidVolumeCommand = Segment(token, 1, 49);
		invalidVolumeCommand["cells"][0]["cell"]["volume_command"] = MAX_VOLCMDS;
		Require(!OK(cap.Call("replace_pattern_segment", invalidVolumeCommand)), "Unadvertised volume command is rejected");
		auto invalidEffectCommand = Segment(token, 1, 49);
		invalidEffectCommand["cells"][0]["cell"]["effect_command"] = MAX_EFFECTS;
		Require(!OK(cap.Call("replace_pattern_segment", invalidEffectCommand)), "Unadvertised effect command is rejected");
		auto invalidEmptyEffect = Segment(token, 1, 49);
		invalidEmptyEffect["cells"][0]["cell"]["effect_parameter"] = 1;
		const auto emptyFailure = cap.Call("replace_pattern_segment", invalidEmptyEffect);
		Require(!OK(emptyFailure) && emptyFailure["error"]["field"] == "effect_parameter", "Empty effect requires a zero parameter");
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
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 1, 49))), "An invalid parameter beside an empty effect can be cleared");
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
		AI::Json editSpecial{{"session", token}, {"channel", 1}, {"first_row", 0}, {"row_count", 1},
			{"cells", AI::Json::array({{{"row", 0}, {"cell", Raw(cell)}}})}};
		editSpecial["cells"][0]["cell"]["effect_command"] = CMD_TEMPO;
		editSpecial["cells"][0]["cell"]["effect_parameter"] = 125;
		Require(OK(cap.Call("replace_pattern_segment", editSpecial)), "Effect columns remain editable beside a preserved special note");
		auto changedSpecial = Segment(token, 1, NOTE_NOTECUT);
		changedSpecial["cells"][0]["cell"]["instrument"] = 1;
		Require(!OK(cap.Call("replace_pattern_segment", changedSpecial)), "Unsupported special-note identity remains protected");
		cap.ForceRelease(); cell = saved;
	}
	{
		AI::PatternCapability cap(*doc, 0, PatternRect(PatternCursor(0, 0), PatternCursor(7, 0, PatternCursor::lastColumn)));
		const auto order = doc->GetSoundFile().Order();
		const auto playOrder = doc->GetSoundFile().GetCurrentOrder();
		const auto request = cap.Call("switch_pattern", {{"pattern", 1}});
		Require(request.value("pending_approval", false), "Session-less Pattern switch waits for approval");
		Require(cap.PendingSwitch() && !cap.PendingExpansion() && cap.Pattern() == 0, "Switch approval is distinct and does not rebind early");
		cap.Tick(AI::PatternCapability::Clock::now() + std::chrono::hours(1));
		Require(cap.PendingSwitch(), "Switch waiting pauses idle timeout");
		Require(ErrorCode(cap.ResolveSwitch(false)) == "patternSwitchRejected" && cap.Pattern() == 0, "Declined switch preserves original binding");
		Require(cap.Call("switch_pattern", {{"pattern", 1}}).value("pending_approval", false), "A declined switch can be requested again");
		const auto switched = cap.ResolveSwitch(true);
		Require(OK(switched) && switched["status"] == "switched" && switched["context"]["pattern"] == 1 && switched.contains("session"), "Approval returns fresh target context and session");
		auto token = switched.at("session");
		Require(cap.Pattern() == 1 && cap.Occupied(), "Approval binds the target Pattern");
		const auto same = cap.Call("switch_pattern", {{"pattern", 1}, {"session", token}});
		Require(OK(same) && same["status"] == "unchanged" && same["session"] == token, "Same target is an authenticated no-op");
		Require(!OK(cap.Call("switch_pattern", {{"pattern", 0}})), "Occupied switching requires a token");
		Require(ErrorCode(cap.Call("switch_pattern", {{"pattern", 0}, {"session", "old-token"}})) == "occupancyLost", "Old token cannot switch");
		Require(!OK(cap.Call("switch_pattern", {{"pattern", 65536}, {"session", token}})) && !cap.PendingSwitch(), "Oversized Pattern indices cannot wrap to an existing target");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 2, 55))), "Switch grants channels outside old selection");
		Require(ErrorCode(cap.Call("switch_pattern", {{"pattern", 0}, {"session", token}})) == "candidateExists", "Unsubmitted edits block switch");
		Require(OK(cap.Call("handoff_for_review", {{"session", token}})), "Switched Pattern freezes its own proposal");
		Require(!OK(cap.Call("switch_pattern", {{"pattern", 0}})) && cap.Pattern() == 1, "Pending proposal blocks switch");
		Require(OK(cap.Reject()), "Rejection clears switched proposal");
		cap.Configure(300, false, true);
		const auto automatic = cap.Call("switch_pattern", {{"pattern", 0}});
		Require(OK(automatic) && automatic["status"] == "switched" && !cap.PendingSwitch(), "Always-allow automatically approves a legal switch");
		token = automatic.at("session");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 2, 57))), "Automatic switch also grants the whole Pattern");
		cap.Call("abort_session", {{"session", token}});
		cap.Configure(300, false, false);
		Require(cap.Call("switch_pattern", {{"pattern", 1}}).value("pending_approval", false), "Manual preference restored");
		cap.ForceRelease();
		Require(!cap.PendingSwitch() && ErrorCode(cap.ResolveSwitch(true)) == "occupancyLost", "Forced release terminates switch waiting");
		Require(doc->GetSoundFile().Order() == order && doc->GetSoundFile().GetCurrentOrder() == playOrder, "Switching never edits Order or playback position");
	}
	{
		auto &sf = doc->GetSoundFile();
		const auto savedOrder = sf.Order();
		const PATTERNINDEX target = sf.Patterns.Size();
		AI::PatternCapability cap(*doc, 0, {});
		Require(cap.Call("switch_pattern", {{"pattern", target}}).value("pending_approval", false),
			"Switching to a missing format-valid Pattern waits for approval");
		Require(cap.SwitchRange()["creates_target"] == true && !sf.Patterns.IsValidPat(target),
			"A pending missing-Pattern switch declares creation without mutating early");
		Require(ErrorCode(cap.ResolveSwitch(false)) == "patternSwitchRejected" && !sf.Patterns.IsValidPat(target),
			"Rejecting the switch does not create a Pattern");
		Require(cap.Call("switch_pattern", {{"pattern", target}}).value("pending_approval", false),
			"The missing Pattern can be requested again");
		const auto created = cap.ResolveSwitch(true);
		Require(OK(created) && created["status"] == "created" && created["created"] == true
			&& created["context"]["pattern"] == target && sf.Patterns.IsValidPat(target),
			"Approval creates and binds the requested Pattern index");
		const ORDERINDEX appended = created["appended_order"].get<ORDERINDEX>();
		Require(appended < sf.Order().size() && sf.Order()[appended] == target,
			"The created Pattern is appended to the effective end of Order");
		cap.Call("abort_session", {{"session", created["session"]}});
		sf.Order() = savedOrder;
		sf.Patterns.Remove(target);
	}
	{
		auto &sf = doc->GetSoundFile();
		const auto p0 = *sf.Patterns[0].GetpModCommand(0, 1);
		const auto p1 = *sf.Patterns[1].GetpModCommand(0, 2);
		doc->GetPatternUndo().ClearUndo();
		AI::PatternCapability cap(*doc, 0, {});
		auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 1, 58))), "Prepare a proposal with default manual acceptance");
		const auto manual = cap.Call("handoff_for_review", {{"session", token}});
		Require(OK(manual) && manual["status"] == "pending_review" && cap.HasProposal()
			&& *sf.Patterns[0].GetpModCommand(0, 1) == p0, "Default handoff stays pending and leaves document unchanged");
		const auto enabled = cap.Configure(300, false, true, true);
		Require(OK(enabled) && enabled["status"] == "applied" && !cap.HasProposal(), "Enabling always-accept immediately applies an existing proposal");
		Require(sf.Patterns[0].GetpModCommand(0, 1)->note == 58 && *sf.Patterns[1].GetpModCommand(0, 2) == p1, "First apply changes only Pattern zero");
		token = cap.Call("switch_pattern", {{"pattern", 1}}).at("session");
		Require(OK(cap.Call("replace_pattern_segment", Segment(token, 2, 60))), "Edit the next Pattern after applying the first");
		const auto automatic = cap.Call("handoff_for_review", {{"session", token}});
		Require(OK(automatic) && automatic["status"] == "applied" && !cap.HasProposal() && !cap.Occupied(), "Later handoffs automatically apply and end occupancy");
		Require(sf.Patterns[0].GetpModCommand(0, 1)->note == 58 && sf.Patterns[1].GetpModCommand(0, 2)->note == 60, "Both independently committed Patterns persist");
		Require(doc->GetPatternUndo().Undo() == 1 && *sf.Patterns[1].GetpModCommand(0, 2) == p1
			&& sf.Patterns[0].GetpModCommand(0, 1)->note == 58, "One Undo restores only the most recently submitted Pattern");
		Require(doc->GetPatternUndo().Undo() == 0 && *sf.Patterns[0].GetpModCommand(0, 1) == p0, "Earlier Pattern has its own Undo step");
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		const auto empty = cap.Call("handoff_for_review", {{"session", token}});
		Require(!OK(empty) && ErrorCode(empty) == "emptyProposal" && empty["status"] == "pending_review" && cap.HasProposal(), "Failed empty automatic apply explicitly retains pending proposal");
		cap.Reject();
		cap.Configure(300, false, true, false);
		token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		cap.Call("replace_pattern_segment", Segment(token, 2, 61));
		cap.Call("handoff_for_review", {{"session", token}});
		const auto original = *sf.Patterns[1].GetpModCommand(0, 0);
		sf.Patterns[1].GetpModCommand(0, 0)->note++;
		const auto revision = doc->AIRevision();
		const auto stale = cap.Configure(300, false, true, true);
		Require(!OK(stale) && ErrorCode(stale) == "stale" && stale["status"] == "pending_review" && cap.HasProposal()
			&& doc->AIRevision() == revision, "Enabling auto-accept retains stale proposal without document changes");
		*sf.Patterns[1].GetpModCommand(0, 0) = original;
		cap.Reject();
		cap.Configure(300, false, false, false);
		Require(cap.Call("switch_pattern", {{"pattern", 0}}).value("pending_approval", false), "Switch approval can wait before enabling its preference");
		const auto allowed = cap.Configure(300, false, true, false);
		Require(OK(allowed) && allowed["status"] == "switched" && allowed.contains("session") && cap.Pattern() == 0, "Enabling always-switch immediately resolves pending switch");
		cap.ForceRelease();
	}
	{
		auto &sf = doc->GetSoundFile();
		const auto target = sf.Patterns.Duplicate(1);
		Require(target != PATTERNINDEX_INVALID, "Create isolated switch revalidation target");
		AI::PatternCapability cap(*doc, 0, {});
		const auto token = cap.Call("get_pattern_context", {{"occupy", true}}).at("session");
		Require(cap.Call("switch_pattern", {{"pattern", target}, {"session", token}}).value("pending_approval", false), "Target exists when switch is requested");
		sf.Patterns.Remove(target);
		const auto disappeared = cap.ResolveSwitch(true);
		Require(!OK(disappeared) && cap.Pattern() == 0 && cap.Occupied(), "Approval revalidates target and retains original binding on failure");
		Require(OK(cap.Call("get_pattern_context", {{"session", token}})), "Failed switch preserves source token");
		Require(cap.Call("switch_pattern", {{"pattern", 1}, {"session", token}}).value("pending_approval", false), "Retained session can request another target after failure");
		const auto switched = cap.ResolveSwitch(true);
		Require(OK(switched) && switched["session"] != token, "Approved rebind replaces token");
		const auto newToken = switched.at("session");
		Require(ErrorCode(cap.Call("get_pattern_context", {{"session", token}})) == "occupancyLost", "Previous Pattern token cannot read new candidate");
		auto lastRow = Segment(newToken, 3, 59);
		lastRow["first_row"] = 127;
		lastRow["row_count"] = 1;
		lastRow["cells"][0]["row"] = 127;
		Require(OK(cap.Call("replace_pattern_segment", lastRow)), "Full Pattern authorization includes final row and channel");
		cap.Call("handoff_for_review", {{"session", newToken}});
		const auto before = cap.Review();
		const auto revision = doc->AIRevision();
		const auto undo = doc->GetPatternUndo().GetUndoName();
		const auto failure = cap.Apply(true, true);
		Require(!OK(failure) && ErrorCode(failure) == "commitFailed" && failure["status"] == "pending_review"
			&& cap.Review() == before && doc->AIRevision() == revision && doc->GetPatternUndo().GetUndoName() == undo,
			"Simulated precommit failure leaves switched proposal pending with revision and Undo intact");
		cap.Reject();
	}
	doc->SetModified(false);
	doc->OnCloseDocument();
}
}
OPENMPT_NAMESPACE_END
#endif
