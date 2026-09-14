#include "stdafx.h"
#include "AIPattern.h"
#include "AICommandNames.h"
#include "EffectInfo.h"
#include "Moddoc.h"
#include "UpdateHints.h"
#include "../soundlib/mod_specifications.h"
#include "../soundlib/AudioCriticalSection.h"
#include "../soundlib/tuning.h"
#include <sstream>

OPENMPT_NAMESPACE_BEGIN
namespace AI
{
static Json WithEndingReminder(Json result)
{
	result["ending_reminder"] = "When finished, handoff_for_review freezes edits; abort_session discards them; release_occupancy ends a read-only session.";
	return result;
}
Json Failure(const char *code, const char *reason, const char *layer)
{
	return {{"ok", false}, {"error", {{"layer", layer}, {"code", code}, {"reason", reason}}}};
}

// Volume column values range 0..64 in every supported module format.
constexpr int MaxVolume = 64;
// Duplicate order indices address one Pattern, so a switch binds the Pattern, not a single occurrence.
constexpr const char *SwitchRelationship = "Repeated order references address the same Pattern; switching binds the Pattern, not one order occurrence.";

static std::string Utf8(const mpt::ustring &value) { return mpt::ToCharset(mpt::Charset::UTF8, value); }
static const char *NoteKind(ModCommand::NOTE note)
{
	if(note == NOTE_NONE) return "empty";
	if(ModCommand::IsNote(note)) return "pitched";
	switch(note)
	{
	case NOTE_KEYOFF: return "note_off";
	case NOTE_NOTECUT: return "note_cut";
	case NOTE_FADE: return "note_fade";
	case NOTE_PC: return "plugin_control";
	case NOTE_PCS: return "plugin_control_smooth";
	default: return "special";
	}
}
static Json Raw(const ModCommand &cell)
{
	return {{"note", cell.note}, {"instrument", cell.instr}, {"volume_command", cell.volcmd},
		{"volume", cell.vol}, {"effect_command", cell.command}, {"effect_parameter", cell.param}};
}
static bool SameRaw(const ModCommand &a, const ModCommand &b)
{
	return a.note == b.note && a.instr == b.instr && a.volcmd == b.volcmd && a.vol == b.vol
		&& a.command == b.command && a.param == b.param;
}
static bool SameCells(const std::vector<ModCommand> &a, const std::vector<ModCommand> &b)
{
	return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), SameRaw);
}
static int Integer(const Json &value, int low, int high)
{
	if(!value.is_number_integer() || value < low || value > high) throw std::invalid_argument("Integer out of range");
	return value.get<int>();
}

PatternCapability::PatternCapability(CModDoc &document, PATTERNINDEX pattern, std::optional<PatternRect> selection)
	: m_doc(document), m_thread(GetCurrentThreadId()), m_pattern(pattern), m_selection(selection) {}
PatternCapability::~PatternCapability() { ForceRelease(); }

Json PatternCapability::Configure(unsigned seconds, bool rangeAlways, bool switchAlways, bool autoAccept)
{
	const bool switchEnabled = switchAlways && !m_alwaysSwitch;
	const bool acceptEnabled = autoAccept && !m_alwaysAccept;
	m_timeout = std::clamp(seconds, 1u, 3600u);
	m_alwaysApprove = rangeAlways;
	m_alwaysSwitch = switchAlways;
	m_alwaysAccept = autoAccept;
	// Turning a preference on resolves work that was already waiting on it. A failed
	// resolution keeps the work pending; later calls never silently retry it.
	if(switchEnabled && m_pendingSwitch) return ResolveSwitch(true);
	if(acceptEnabled && m_proposal) return Apply();
	return {{"ok", true}};
}
void PatternCapability::ForceRelease()
{
	m_retained = false;
	if(m_ownsOccupancy) m_doc.SetAIOccupied(false);
	m_ownsOccupancy = false;
	m_token.clear();
	m_pending.reset();
	m_pendingSwitch.reset();
	if(!m_proposal) { m_candidate.clear(); m_baseline.clear(); }
}
void PatternCapability::Tick(Clock::time_point now)
{
	if(m_retained && !m_pending && !m_pendingSwitch && now >= m_deadline) ForceRelease();
}
void PatternCapability::Capture()
{
	Adopt(CapturePattern(m_pattern, m_selection));
}
PatternCapability::CaptureState PatternCapability::CapturePattern(PATTERNINDEX pattern, std::optional<PatternRect> selection) const
{
	auto &sf = m_doc.GetSoundFile();
	if(!sf.Patterns.IsValidPat(pattern)) throw std::invalid_argument("Bound Pattern no longer exists");
	CaptureState state;
	state.pattern = pattern;
	state.rows = sf.Patterns[pattern].GetNumRows();
	state.channels = sf.GetNumChannels();
	const auto *first = sf.Patterns[pattern].GetpModCommand(0, 0);
	state.baseline.assign(first, first + size_t(state.rows) * state.channels);
	state.candidate = state.baseline;
	state.envelope = selection.value_or(PatternRect(PatternCursor(0, 0), PatternCursor(state.rows - 1, state.channels - 1, PatternCursor::lastColumn)));
	state.envelope.Sanitize(state.rows, state.channels);
	state.signature = Signature(pattern);
	GUID id{};
	if(FAILED(CoCreateGuid(&id))) throw std::runtime_error("Cannot create session identity");
	wchar_t text[40]{};
	StringFromGUID2(id, text, 40);
	state.token = Utf8(mpt::ToUnicode(text));
	return state;
}
void PatternCapability::Adopt(CaptureState state)
{
	m_pattern = state.pattern;
	m_rows = state.rows;
	m_channels = state.channels;
	m_envelope = state.envelope;
	m_baseline = std::move(state.baseline);
	m_candidate = std::move(state.candidate);
	m_signature = std::move(state.signature);
	m_token = std::move(state.token);
}

std::string PatternCapability::Signature() const { return Signature(m_pattern); }
std::string PatternCapability::Signature(PATTERNINDEX patternIndex) const
{
	const auto &sf = m_doc.GetSoundFile();
	if(!sf.Patterns.IsValidPat(patternIndex)) return {};
	const auto &pattern = sf.Patterns[patternIndex];
	std::ostringstream data(std::ios::binary);
	// Exact internal dependency record, never sent to the Agent. No hash collisions.
	data << sf.GetType() << ':' << sf.GetNumChannels() << ':' << pattern.GetNumRows() << ':'
		<< sf.Order().GetDefaultTempo().GetRaw() << ':' << sf.Order().GetDefaultSpeed() << ':'
		<< sf.m_SongFlags.GetRaw() << ':' << sf.m_nDefaultRowsPerBeat << ':' << sf.m_nDefaultRowsPerMeasure << ':' << int(sf.m_nTempoMode)
		<< ':' << pattern.GetRowsPerBeat() << ':' << pattern.GetRowsPerMeasure() << ':';
	for(const auto swing : sf.m_tempoSwing) data << swing << ',';
	for(const auto swing : pattern.GetTempoSwing()) data << swing << ',';
	data.write(reinterpret_cast<const char *>(pattern.GetpModCommand(0, 0)), size_t(pattern.GetNumRows()) * sf.GetNumChannels() * sizeof(ModCommand));
	// Every resource is summarized in Score Context, so those summaries are dependencies.
	for(SAMPLEINDEX i = 1; i <= sf.GetNumSamples(); ++i)
	{
		const auto &sample = sf.GetSample(i);
		data << i << ':' << sf.GetSampleName(i) << ':';
		data.write(reinterpret_cast<const char *>(&sample), sizeof(sample));
		if(sample.samplev()) data.write(static_cast<const char *>(sample.samplev()), sample.GetSampleSizeInBytes());
	}
	for(INSTRUMENTINDEX i = 1; i <= sf.GetNumInstruments(); ++i)
	{
		data << i << ':';
		if(sf.Instruments[i])
		{
			sf.SaveITIInstrument(i, data, {}, false, false);
			sf.SaveExtendedInstrumentProperties(i, MOD_TYPE_MPT, data);
			if(sf.Instruments[i]->pTuning) sf.Instruments[i]->pTuning->Serialize(data);
		}
	}
	return data.str();
}

Json PatternCapability::Context(const std::vector<ModCommand> &cells, const Json &args) const
{
	return ContextFor(m_pattern, m_rows, m_channels, cells, args);
}

// Uses an explicit state so a reply can be prepared before a new binding is adopted.
Json PatternCapability::ContextFor(PATTERNINDEX patternIndex, ROWINDEX rowCount, CHANNELINDEX channelCount, const std::vector<ModCommand> &cells, const Json &args) const
{
	const auto &sf = m_doc.GetSoundFile();
	int row = 0, rows = rowCount, channel = 0, channels = channelCount;
	if(args.contains("range"))
	{
		const auto &range = args.at("range");
		row = Integer(range.at("first_row"), 0, rowCount - 1);
		rows = Integer(range.at("row_count"), 1, rowCount - row);
		channel = Integer(range.at("first_channel"), 0, channelCount - 1);
		channels = Integer(range.at("channel_count"), 1, channelCount - channel);
	}
	Json sparse = Json::array(), instruments = Json::array(), samples = Json::array();
	for(int r = row; r < row + rows; ++r)
		for(int c = channel; c < channel + channels; ++c)
		{
			const auto &cell = cells[size_t(r) * channelCount + c];
			if(SameRaw(cell, ModCommand{})) continue;
			sparse.push_back({{"row", r}, {"channel", c}, {"raw", Raw(cell)}, {"note_kind", NoteKind(cell.note)},
				{"note_name", Utf8(sf.GetNoteName(cell.note, cell.instr))}, {"volume_command_name", SemanticName(cell.volcmd)}, {"effect_command_name", SemanticName(cell.command)}, {"instrument_reference", cell.instr}});
		}
	for(INSTRUMENTINDEX i = 1; i <= sf.GetNumInstruments(); ++i)
		if(sf.Instruments[i]) instruments.push_back({{"id", i}, {"name", Utf8(mpt::ToUnicode(sf.GetCharsetInternal(), sf.GetInstrumentName(i)))}});
	for(SAMPLEINDEX i = 1; i <= sf.GetNumSamples(); ++i)
		samples.push_back({{"id", i}, {"name", Utf8(mpt::ToUnicode(sf.GetCharsetInternal(), sf.GetSampleName(i)))}, {"frames", sf.GetSample(i).nLength}});
	const auto &spec = sf.GetModSpecifications();
	const EffectInfo effectInfo(sf);
	Json volumeCommands = Json::array({{{"id", VOLCMD_NONE}, {"name", SemanticName(VOLCMD_NONE)}, {"parameter_min", 0}, {"parameter_max", 0}}});
	for(int command = VOLCMD_NONE + 1; command < MAX_VOLCMDS; ++command)
	{
		const auto volcmd = static_cast<ModCommand::VOLCMD>(command);
		if(!spec.HasVolCommand(volcmd)) continue;
		ModCommand::VOL minimum = 0, maximum = 0;
		const auto index = effectInfo.GetIndexFromVolCmd(volcmd);
		if(index < 0 || !effectInfo.GetVolCmdInfo(static_cast<UINT>(index), nullptr, &minimum, &maximum)) continue;
		volumeCommands.push_back({{"id", command}, {"name", SemanticName(volcmd)}, {"parameter_min", minimum}, {"parameter_max", maximum}});
	}
	Json effectCommands = Json::array({{{"id", CMD_NONE}, {"name", SemanticName(CMD_NONE)}, {"parameter_min", 0}, {"parameter_max", 0}}});
	for(int command = CMD_NONE + 1; command < MAX_EFFECTS; ++command)
	{
		const auto effect = static_cast<ModCommand::COMMAND>(command);
		if(spec.HasCommand(effect)) effectCommands.push_back({{"id", command}, {"name", SemanticName(effect)}, {"parameter_min", 0}, {"parameter_max", 255}});
	}
	const auto &pattern = sf.Patterns[patternIndex];
	return {{"pattern", patternIndex}, {"rows", rowCount}, {"channels", channelCount}, {"cells", sparse},
		{"range", {{"first_row", row}, {"row_count", rows}, {"first_channel", channel}, {"channel_count", channels}}},
		{"timing", {{"default_tempo", sf.Order().GetDefaultTempo().ToDouble()}, {"default_speed", sf.Order().GetDefaultSpeed()},
			{"tempo_mode", int(sf.m_nTempoMode)}, {"rows_per_beat", pattern.GetOverrideSignature() ? pattern.GetRowsPerBeat() : sf.m_nDefaultRowsPerBeat},
			{"rows_per_measure", pattern.GetOverrideSignature() ? pattern.GetRowsPerMeasure() : sf.m_nDefaultRowsPerMeasure}}},
		{"format", {{"name", spec.fileExtension}, {"note_min", spec.noteMin}, {"note_max", spec.noteMax}, {"note_off", spec.hasNoteOff},
			{"volume_max", spec.HasVolCommand(VOLCMD_VOLUME) ? MaxVolume : 0}, {"volume_commands", std::move(volumeCommands)},
			{"effect_commands", std::move(effectCommands)}, {"rows_max", spec.patternRowsMax}, {"channels_max", spec.channelsMax}}},
		{"instruments", instruments}, {"samples", samples}};
}

Json PatternCapability::PatternOrder() const
{
	const auto &sf = m_doc.GetSoundFile();
	const auto &sequence = sf.Order();
	const auto patternInfo = [&](PATTERNINDEX pattern)
	{
		const auto &reference = sf.Patterns[pattern];
		return Json{{"pattern", pattern}, {"name", Utf8(mpt::ToUnicode(sf.GetCharsetInternal(), reference.GetName()))}, {"rows", reference.GetNumRows()}};
	};
	Json entries = Json::array();
	for(ORDERINDEX order = 0; order < sequence.GetLength(); ++order)
	{
		const PATTERNINDEX pattern = sequence[order];
		Json entry{{"order", order}, {"kind", "pattern"}, {"pattern", pattern}};
		if(sf.Patterns.IsValidPat(pattern))
		{
			const auto info = patternInfo(pattern);
			entry["name"] = info["name"];
			entry["rows"] = info["rows"];
		} else if(pattern == PATTERNINDEX_SKIP) entry["kind"] = "skip";
		else if(pattern == PATTERNINDEX_INVALID) entry["kind"] = "stop";
		else entry["kind"] = "invalid";
		entries.push_back(std::move(entry));
	}
	Json unreferenced = Json::array();
	for(PATTERNINDEX pattern = 0; pattern < sf.Patterns.Size(); ++pattern)
		if(sf.Patterns.IsValidPat(pattern) && sequence.FindOrder(pattern) == ORDERINDEX_INVALID) unreferenced.push_back(patternInfo(pattern));
	return {{"ok", true},
		{"sequence", {{"index", sf.Order.GetCurrentSequenceIndex()}, {"name", Utf8(sequence.GetName())}}},
		{"entries", entries}, {"unreferenced_patterns", unreferenced}};
}

Json PatternCapability::PatternInfo(PATTERNINDEX pattern) const
{
	const auto &sf = m_doc.GetSoundFile();
	Json info{{"pattern", pattern}};
	if(sf.Patterns.IsValidPat(pattern))
	{
		const auto &reference = sf.Patterns[pattern];
		info["name"] = Utf8(mpt::ToUnicode(sf.GetCharsetInternal(), reference.GetName()));
		info["rows"] = reference.GetNumRows();
	}
	return info;
}

Json PatternCapability::OrderReferences(PATTERNINDEX pattern) const
{
	const auto &sequence = m_doc.GetSoundFile().Order();
	Json orders = Json::array();
	for(ORDERINDEX order = 0; order < sequence.GetLength(); ++order)
		if(sequence[order] == pattern) orders.push_back(order);
	return {{"pattern", pattern}, {"orders", orders}, {"count", orders.size()}, {"repeated", orders.size() > 1},
		{"message", orders.size() > 1 ? "All listed order indices address the same Pattern." : "This Pattern has a single order reference."}};
}

Json PatternCapability::Call(const std::string &tool, const Json &args)
{
	if(GetCurrentThreadId() != m_thread) return Failure("owningThreadRequired", "Call on the document owning thread");
	if(m_callActive) return Failure("busy", "Another capability call is executing");
	m_callActive = true;
	struct EndCall
	{
		PatternCapability &capability;
		~EndCall()
		{
			capability.m_callActive = false;
			// A waiting switch (including a session-less one with a reservation) must survive the call.
			if(!capability.m_retained && !capability.m_pending && !capability.m_pendingSwitch) capability.ForceRelease();
		}
	} end{*this};
	// Sole reminder site for capability tool results: every result gains ending_reminder exactly once here.
	return WithEndingReminder(Dispatch(tool, args));
}

Json PatternCapability::Dispatch(const std::string &tool, const Json &args)
{
	Tick();
	try
	{
		if(!args.is_object()) return Failure("schemaFailure", "Arguments must be an object");
		if(tool != "get_pattern_context" && tool != "get_pattern_order" && tool != "switch_pattern" && tool != "replace_pattern_segment"
			&& tool != "handoff_for_review" && tool != "abort_session" && tool != "release_occupancy") return Failure("unsupported", "Unknown Pattern tool");
		if(args.contains("occupy") && !args.at("occupy").is_boolean()) return Failure("schemaFailure", "occupy must be boolean");
		if(tool != "switch_pattern" && args.contains("pattern") && (!args.at("pattern").is_number_integer() || args.at("pattern").get<int>() != static_cast<int>(m_pattern)))
			return Failure("boundPatternViolation", "A session cannot read or write another Pattern");
		// Order inspection is a session-less read: it never captures or occupies the document.
		if(tool == "get_pattern_order") return PatternOrder();
		if(m_pending) return Failure("approvalPending", "Resolve the pending expansion first");
		if(m_pendingSwitch) return Failure("approvalPending", "Resolve the pending Pattern switch first");
		if(tool == "switch_pattern") return Switch(args);
		if(args.contains("session"))
		{
			if(!m_retained || args.at("session") != m_token) return Failure("occupancyLost", "Session ended or was released");
			if(Signature() != m_signature) { ForceRelease(); return Failure("stale", "Bound Pattern dependencies changed"); }
		} else if(tool != "get_pattern_context") return Failure("occupancyLost", "A retained session is required");
		else
		{
			if(m_retained || m_proposal || m_doc.AIOccupied()) return Failure("busy", "Finish the existing session or proposal first");
			m_doc.SetAIOccupied(true);
			m_ownsOccupancy = true;
			Capture();
		}
		if(tool == "get_pattern_context")
		{
			auto result = Json{{"ok", true}, {"context", Context(args.value("baseline", false) ? m_baseline : m_candidate, args)}};
			if(args.value("occupy", false) || m_retained)
			{
				m_retained = true;
				m_doc.SetAIOccupied(true);
				m_deadline = Clock::now() + std::chrono::seconds(m_timeout);
				result["session"] = m_token;
			} else ForceRelease();
			return result;
		}
		if(tool == "abort_session" || tool == "release_occupancy")
		{
			if(tool == "release_occupancy" && !SameCells(m_candidate, m_baseline)) return Failure("candidateExists", "Handoff or abort edited candidates");
			ForceRelease();
			return {{"ok", true}};
		}
		if(tool == "replace_pattern_segment") return Replace(args);
		if(tool == "handoff_for_review")
		{
			auto diff = Diff(m_baseline, m_candidate);
			m_proposal = true;
			ForceRelease();
			if(m_alwaysAccept) return Apply();
			return {{"ok", true}, {"status", "pending_review"}, {"diff", std::move(diff)}};
		}
		return Failure("unsupported", "Unknown Pattern tool");
	} catch(const std::exception &)
	{
		return Failure("validationFailure", "Invalid request or Pattern state");
	}
}

Json PatternCapability::Diff(const std::vector<ModCommand> &before, const std::vector<ModCommand> &after) const
{
	Json result = Json::array();
	for(size_t i = 0; i < before.size(); ++i)
		if(!SameRaw(before[i], after[i])) result.push_back({{"row", i / m_channels}, {"channel", i % m_channels}, {"before", Raw(before[i])}, {"after", Raw(after[i])}});
	return result;
}

Json PatternCapability::Validate(const ModCommand &before, const ModCommand &after, ROWINDEX row) const
{
	const auto &sf = m_doc.GetSoundFile();
	const auto &spec = sf.GetModSpecifications();
	const EffectInfo effectInfo(sf);
	auto error = [&](const char *field, int value, const char *reason)
	{
		auto result = Failure("validationFailure", reason);
		result["error"]["row"] = row;
		result["error"]["field"] = field;
		result["error"]["value"] = value;
		return result;
	};
	if(SameRaw(before, after)) return {{"ok", true}};
	if(before.IsPcNote()) return error("note", after.note, "PC/PCS cells must be preserved byte-for-byte");
	const bool supportedBefore = before.note == NOTE_NONE || before.IsNote() || before.note == NOTE_KEYOFF;
	if(!supportedBefore && (after.note != before.note || after.instr != before.instr))
		return error("note", after.note, "Preserve the note and instrument of unsupported special-note cells");
	if(after.note != before.note)
	{
		const bool validNote = after.note == NOTE_NONE || (after.IsNote() && spec.HasNote(after.note)) || (after.note == NOTE_KEYOFF && spec.hasNoteOff);
		if(!validNote) return error("note", after.note, "Only format-supported pitched notes, empty notes and note-offs can be written");
	}
	if(after.instr != before.instr && after.instr && (sf.GetNumInstruments() ? (after.instr > sf.GetNumInstruments() || !sf.Instruments[after.instr]) : after.instr > sf.GetNumSamples()))
		return error("instrument", after.instr, "Instrument/sample reference does not exist");
	if(after.command != before.command || after.param != before.param)
	{
		if(after.command == CMD_NONE)
		{
			if(after.param != 0) return error("effect_parameter", after.param, "An empty effect column must have parameter zero");
		} else if(!spec.HasCommand(after.command))
		{
			return error("effect_command", after.command, "Effect command is not supported by the module format");
		}
	}
	if(after.volcmd != before.volcmd || after.vol != before.vol)
	{
		if(after.volcmd == VOLCMD_NONE)
		{
			if(after.vol != 0) return error("volume", after.vol, "An empty volume column must have parameter zero");
		} else
		{
			if(!spec.HasVolCommand(after.volcmd)) return error("volume_command", after.volcmd, "Volume command is not supported by the module format");
			ModCommand::VOL minimum = 0, maximum = 0;
			const auto index = effectInfo.GetIndexFromVolCmd(after.volcmd);
			if(index < 0 || !effectInfo.GetVolCmdInfo(static_cast<UINT>(index), nullptr, &minimum, &maximum))
				return error("volume_command", after.volcmd, "Volume command is not editable in the module format");
			if(after.vol < minimum || after.vol > maximum) return error("volume", after.vol, "Volume command parameter is outside the format-supported range");
		}
	}
	return {{"ok", true}};
}

Json PatternCapability::Replace(const Json &args, bool approved)
{
	int first = 0, count = 0, channel = 0;
	try
	{
		first = Integer(args.at("first_row"), 0, m_rows - 1);
		count = Integer(args.at("row_count"), 1, m_rows - first);
		channel = Integer(args.at("channel"), 0, m_channels - 1);
	} catch(const std::exception &) { return Failure("validationFailure", "Invalid contiguous segment range"); }
	std::vector<ModCommand> desired(count);
	std::vector<bool> supplied(count);
	if(!args.contains("cells") || !args.at("cells").is_array()) return Failure("validationFailure", "cells must be a sparse array");
	for(const auto &entry : args.at("cells"))
	{
		int row = first;
		const char *field = "row";
		try
		{
			row = Integer(entry.at("row"), first, first + count - 1);
			if(supplied[row - first]) throw std::invalid_argument("Duplicate row");
			supplied[row - first] = true;
			const auto &cell = entry.at("cell");
			if(!cell.is_object() || cell.size() != 6) throw std::invalid_argument("Expected all six raw cell fields");
			auto &d = desired[row - first];
			auto value = [&](const char *key) { field = key; return Integer(cell.at(key), 0, 255); };
			d.note = static_cast<ModCommand::NOTE>(value("note"));
			d.instr = static_cast<ModCommand::INSTR>(value("instrument"));
			d.volcmd = static_cast<ModCommand::VOLCMD>(value("volume_command"));
			d.vol = static_cast<ModCommand::VOL>(value("volume"));
			d.command = static_cast<ModCommand::COMMAND>(value("effect_command"));
			d.param = static_cast<ModCommand::PARAM>(value("effect_parameter"));
		} catch(const std::exception &)
		{
			auto result = Failure("validationFailure", "Invalid, missing or duplicate raw cell field");
			result["error"]["row"] = row;
			result["error"]["field"] = field;
			result["error"]["value"] = entry;
			return result;
		}
	}
	for(int i = 0; i < count; ++i)
	{
		auto validation = Validate(m_candidate[size_t(first + i) * m_channels + channel], desired[i], static_cast<ROWINDEX>(first + i));
		if(!validation["ok"].get<bool>()) return validation;
	}
	const bool expansion = first < static_cast<int>(m_envelope.GetStartRow()) || first + count - 1 > static_cast<int>(m_envelope.GetEndRow())
		|| channel < static_cast<int>(m_envelope.GetStartChannel()) || channel > static_cast<int>(m_envelope.GetEndChannel());
	if(expansion && !approved && !m_alwaysApprove)
	{
		m_pending = args;
		return {{"ok", true}, {"pending_approval", true}};
	}
	auto candidate = m_candidate;
	Json cells = Json::array(), diff = Json::array();
	for(int i = 0; i < count; ++i)
	{
		const size_t offset = size_t(first + i) * m_channels + channel;
		cells.push_back({{"row", first + i}, {"cell", Raw(desired[i])}});
		if(!SameRaw(candidate[offset], desired[i])) diff.push_back({{"row", first + i}, {"channel", channel}, {"before", Raw(candidate[offset])}, {"after", Raw(desired[i])}});
		candidate[offset] = desired[i];
	}
	Json result{{"ok", true}, {"cells", cells}, {"diff", diff}};
	if(expansion) m_envelope = PatternRect(PatternCursor(static_cast<ROWINDEX>(std::min(first, static_cast<int>(m_envelope.GetStartRow()))), static_cast<CHANNELINDEX>(std::min(channel, static_cast<int>(m_envelope.GetStartChannel())))),
		PatternCursor(static_cast<ROWINDEX>(std::max(first + count - 1, static_cast<int>(m_envelope.GetEndRow()))), static_cast<CHANNELINDEX>(std::max(channel, static_cast<int>(m_envelope.GetEndChannel()))), PatternCursor::lastColumn));
	m_candidate.swap(candidate);
	m_deadline = Clock::now() + std::chrono::seconds(m_timeout);
	return result;
}

Json PatternCapability::ResolveExpansion(bool approve)
{
	if(GetCurrentThreadId() != m_thread) return Failure("owningThreadRequired", "Use owning thread");
	if(!m_pending || !m_retained) return Failure("occupancyLost", "No pending occupied request");
	auto args = std::move(*m_pending);
	m_pending.reset();
	m_deadline = Clock::now() + std::chrono::seconds(m_timeout);
	if(Signature() != m_signature) { ForceRelease(); return Failure("stale", "Dependencies changed while waiting"); }
	if(!approve) return Failure("rangeRejected", "Owner declined the range expansion");
	return WithEndingReminder(Replace(args, true));
}

Json PatternCapability::Switch(const Json &args)
{
	auto &sf = m_doc.GetSoundFile();
	if(!args.contains("pattern") || !args.at("pattern").is_number_integer())
		return Failure("schemaFailure", "switch_pattern requires an integer pattern");
	// Bounds-check the full JSON integer before narrowing; 65536 must never wrap to Pattern 0.
	long long requested = 0;
	try { requested = args.at("pattern").get<long long>(); }
	catch(const std::exception &) { return Failure("validationFailure", "Target Pattern index is out of range"); }
	if(requested < 0 || static_cast<unsigned long long>(requested) >= static_cast<unsigned long long>(sf.Patterns.Size())
		|| !sf.Patterns.IsValidPat(static_cast<PATTERNINDEX>(requested)))
		return Failure("validationFailure", "Target Pattern does not exist");
	const PATTERNINDEX target = static_cast<PATTERNINDEX>(requested);
	if(args.contains("session") && !args.at("session").is_string())
		return Failure("schemaFailure", "session must be a string");
	const bool authenticated = args.contains("session");
	if(authenticated)
	{
		if(!m_retained || args.at("session").get<std::string>() != m_token)
			return Failure("occupancyLost", "Session ended or was released");
		if(Signature() != m_signature) { ForceRelease(); return Failure("stale", "Bound Pattern dependencies changed"); }
	} else
	{
		if(m_retained) return Failure("occupancyLost", "Switching the bound Pattern requires its session token");
		if(m_proposal) return Failure("busy", "Finish the existing proposal before switching");
		if(m_doc.AIOccupied()) return Failure("busy", "Another AI session holds the document");
	}
	if(target == m_pattern)
	{
		Json result{{"ok", true}, {"status", "unchanged"}, {"source", PatternInfo(m_pattern)}, {"target", PatternInfo(target)},
			{"order_references", {{"source", OrderReferences(m_pattern)}, {"target", OrderReferences(target)}}},
			{"relationship", SwitchRelationship}};
		if(authenticated)
		{
			m_deadline = Clock::now() + std::chrono::seconds(m_timeout);
			result["context"] = Context(m_candidate, Json::object());
			result["session"] = m_token;
		}
		return result;
	}
	if(authenticated && !SameCells(m_candidate, m_baseline)) return Failure("candidateExists", "Handoff or abort edited candidates before switching");
	SwitchRequest request;
	request.source = m_pattern;
	request.target = target;
	request.sourceSignature = Signature();
	request.token = m_token;
	request.authenticated = authenticated;
	if(!authenticated)
	{
		// Reserve ownership while waiting so another facade cannot take over the document.
		m_doc.SetAIOccupied(true);
		m_ownsOccupancy = true;
	}
	if(!m_alwaysSwitch)
	{
		m_pendingSwitch = std::move(request);
		return {{"ok", true}, {"pending_approval", true}};
	}
	return PerformSwitch(request);
}

Json PatternCapability::PerformSwitch(const SwitchRequest &request)
{
	auto &sf = m_doc.GetSoundFile();
	if(!sf.Patterns.IsValidPat(request.target))
	{
		if(!request.authenticated) ForceRelease();
		return Failure("validationFailure", "Target Pattern disappeared while waiting");
	}
	CaptureState captured;
	try
	{
		captured = CapturePattern(request.target, std::nullopt);
	} catch(const std::exception &)
	{
		// A failed capture never replaced the source binding; only a session-less reservation needs cleanup.
		if(!request.authenticated) ForceRelease();
		return Failure("validationFailure", "Target Pattern cannot be captured");
	}
	// Build the complete reply from the temporary capture before adopting it, so a failed
	// allocation cannot leave the original session half-replaced.
	Json reply;
	try
	{
		reply = Json{{"ok", true}, {"status", "switched"},
			{"context", ContextFor(captured.pattern, captured.rows, captured.channels, captured.candidate, Json::object())},
			{"session", captured.token}, {"source", PatternInfo(request.source)}, {"target", PatternInfo(request.target)},
			{"order_references", {{"source", OrderReferences(request.source)}, {"target", OrderReferences(request.target)}}},
			{"relationship", SwitchRelationship}};
	} catch(const std::exception &)
	{
		if(!request.authenticated) ForceRelease();
		return Failure("commitFailed", "Could not prepare the switch response");
	}
	m_doc.SetAIOccupied(true);
	m_ownsOccupancy = true;
	Adopt(std::move(captured));
	m_retained = true;
	m_deadline = Clock::now() + std::chrono::seconds(m_timeout);
	return reply;
}

Json PatternCapability::ResolveSwitch(bool approve)
{
	if(GetCurrentThreadId() != m_thread) return Failure("owningThreadRequired", "Use owning thread");
	if(!m_pendingSwitch) return Failure("occupancyLost", "No pending Pattern switch");
	SwitchRequest request = std::move(*m_pendingSwitch);
	m_pendingSwitch.reset();
	if(request.authenticated)
	{
		if(!m_retained || m_token != request.token) { ForceRelease(); return WithEndingReminder(Failure("occupancyLost", "Session ended while waiting")); }
		if(Signature() != request.sourceSignature) { ForceRelease(); return WithEndingReminder(Failure("stale", "Source Pattern changed while waiting")); }
	} else if(!m_ownsOccupancy || !m_doc.AIOccupied())
	{
		return WithEndingReminder(Failure("occupancyLost", "Reservation was released while waiting"));
	}
	if(!approve)
	{
		// A retained session keeps its binding and token; a session-less reservation is released.
		if(request.authenticated) m_deadline = Clock::now() + std::chrono::seconds(m_timeout);
		else ForceRelease();
		return WithEndingReminder(Failure("patternSwitchRejected", "Owner declined the Pattern switch"));
	}
	return WithEndingReminder(PerformSwitch(request));
}

Json PatternCapability::SwitchRange() const
{
	if(!m_pendingSwitch) return Json::object();
	const auto &request = *m_pendingSwitch;
	const auto source = PatternInfo(request.source);
	const auto target = PatternInfo(request.target);
	return {{"source_pattern", request.source},
		{"source_name", source.value("name", std::string{})},
		{"target_pattern", request.target},
		{"target_name", target.value("name", std::string{})},
		{"target_rows", target.value("rows", 0)},
		{"target_channels", m_doc.GetSoundFile().GetNumChannels()},
		{"authenticated", request.authenticated},
		{"order_references", {{"source", OrderReferences(request.source)}, {"target", OrderReferences(request.target)}}},
		{"relationship", SwitchRelationship}};
}

Json PatternCapability::ExpansionRange() const
{
	if(!m_pending) return Json::object();
	return {{"first_row", m_pending->at("first_row")}, {"last_row", m_pending->at("first_row").get<int>() + m_pending->at("row_count").get<int>() - 1},
		{"channel", m_pending->at("channel")}, {"grant_first_row", m_envelope.GetStartRow()}, {"grant_last_row", m_envelope.GetEndRow()},
		{"grant_first_channel", m_envelope.GetStartChannel()}, {"grant_last_channel", m_envelope.GetEndChannel()}};
}

Json PatternCapability::Review() const
{
	if(GetCurrentThreadId() != m_thread) return Failure("owningThreadRequired", "Use owning thread");
	if(!m_proposal) return Failure("noProposal", "No proposal available");
	return {{"ok", true}, {"pattern", m_pattern}, {"status", Signature() == m_signature ? "current" : "stale"},
		{"diff", Diff(m_baseline, m_candidate)}};
}

Json PatternCapability::Reject()
{
	if(GetCurrentThreadId() != m_thread) return Failure("owningThreadRequired", "Use owning thread");
	m_proposal = false;
	ForceRelease();
	return {{"ok", true}};
}

Json PatternCapability::Apply(bool whole, bool simulateFailure)
{
	// Every rejected attempt keeps the frozen proposal pending for the owner.
	auto pending = [](Json result) { result["status"] = "pending_review"; return result; };
	if(GetCurrentThreadId() != m_thread) return pending(Failure("owningThreadRequired", "Use owning thread"));
	if(!whole) return pending(Failure("unsupported", "Partial acceptance is unavailable"));
	if(!m_proposal) return pending(Failure("noProposal", "No proposal available"));
	if(m_doc.AIOccupied()) return pending(Failure("busy", "Release occupancy before Apply"));
	if(Signature() != m_signature) return pending(Failure("stale", "Dependencies changed; request a fresh proposal"));
	for(size_t i = 0; i < m_candidate.size(); ++i)
	{
		auto result = Validate(m_baseline[i], m_candidate[i], static_cast<ROWINDEX>(i / m_channels));
		if(!result["ok"].get<bool>()) return pending(std::move(result));
	}
	if(simulateFailure) return pending(Failure("commitFailed", "Simulated precommit allocation failure"));
	if(SameCells(m_baseline, m_candidate)) return pending(Failure("emptyProposal", "Proposal has no changes"));
	// Everything that can allocate for our result happens before native Undo preparation.
	Json result{{"ok", true}, {"status", "applied"}};
	if(!m_doc.GetPatternUndo().PrepareUndo(m_pattern, 0, 0, m_channels, m_rows, "Apply AI proposal"))
		return pending(Failure("commitFailed", "Could not allocate native Undo snapshot"));
	{
		CriticalSection guard;
		std::copy(m_candidate.begin(), m_candidate.end(), m_doc.GetSoundFile().Patterns[m_pattern].GetpModCommand(0, 0));
	}
	m_doc.SetModified();
	m_doc.UpdateAllViews(nullptr, PatternHint(m_pattern).Data());
	m_proposal = false;
	ForceRelease();
	return result;
}
}
OPENMPT_NAMESPACE_END
