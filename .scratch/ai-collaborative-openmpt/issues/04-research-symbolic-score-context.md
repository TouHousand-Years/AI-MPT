# Evaluate symbolic Score Context representations

Parent: ../map.md
Type: research
Status: resolved
Blocked by:

## Question

Which symbolic representations can faithfully and efficiently convey an OpenMPT project or fragment to multimodal AI models, including tracker-specific events that MIDI or conventional notation cannot express, while remaining revision-bound and suitable for MCP resources and tool results?

## Comments

## Answer

### Decision

Use a versioned, sparse **OpenMPT Score Context JSON** as the canonical AI representation. It should be a semantic projection of the pinned OpenMPT baseline, not a dump of C++ memory and not a piano-roll model. Preserve the pattern grid and commands exactly enough to round-trip a selected fragment: orders/pattern identity, row and channel coordinates, note or special-note value, instrument, volume command/value, effect command/parameter, pattern signature and swing, plus the project-level timing and compatibility state needed to interpret them.

**The Tracker event vocabulary and Pattern structure reuse the original OpenMPT `ModCommand` and `CPattern` implementation. The optional clipboard projection reuses the original OpenMPT `PatternClipboard` serializer and parser.** The project adds only the revision-bound semantic JSON mapping, structural range identity, completeness/provenance envelope, and MCP presentation; it does not redefine Tracker commands or fork the clipboard grammar.

Offer OpenMPT clipboard text as an optional compact projection for prompt display and debugging. Offer MIDI and MusicXML only as explicitly **lossy derived views** for models or external systems that benefit from those vocabularies; never accept either as authoritative evidence of the current project state.

### Comparison

| Representation | Tracker fidelity | Context/token efficiency | Provenance and MCP fit | Role |
| --- | --- | --- | --- | --- |
| Sparse semantic JSON | Highest when its schema mirrors `CPattern` / `ModCommand` and includes timing/compatibility context. It can name special notes, volume commands and every effect command without format-letter ambiguity. | Good for sparse fragments if empty cells and repeated defaults are omitted; verbose keys can be controlled with a documented compact event tuple projection. Unlike clipboard text, it need not spend characters on every empty cell. | Best: schema-versioned, validates as structured tool output, and can also be served as `application/json`. An envelope can carry an opaque revision and exact range. | Canonical representation and mutation precondition. |
| OpenMPT pattern clipboard/text | High for copied pattern cells: the implementation serializes note, instrument, volume column and effect/parameter, and multi-pattern copy can include orders, rows, name, signature and swing. Effect letters remain source-format-dependent. | Very compact and familiar for dense tracker data, but fixed-width cells encode empty columns/cells and become wasteful for sparse or wide selections. It has no self-describing field names. | Plain MCP text is easy, but the format has no project identity, revision, stable range locator, schema version, instrument/sample definitions, or complete project state. | Optional compact/readable projection; never canonical on its own. |
| Standard MIDI | Good for performed note timing, velocity, channel, controllers, programs and tempo-oriented interchange. | Usually compact in binary, but binary is unsuitable for direct model context and a JSON/text expansion removes much of that advantage. | Can be an MCP blob or derived text/JSON, but cannot serve as a lossless revision snapshot. | Optional performance-oriented export/import projection. |
| MusicXML | Strong for conventional notated pitch, duration, measure, meter, articulation and print-score semantics. | Poor for tracker context: XML is verbose and translating rows/effect execution into conventional durations and notation adds inferred data. | Can be a text resource, but tracker extensions would be project-specific and therefore weakly interoperable. | Optional notation-oriented derived view, not the editing contract. |

OpenMPT's own source establishes why MIDI or notation cannot be canonical. A `ModCommand` is one pattern cell containing note, instrument, a volume command/value and an effect command/parameter; its special notes distinguish key-off, immediate cut, fade, and two plugin-parameter-control modes. Its effect enum includes pattern/order control, retrigger, speed/tempo, sample offset, effect-memory-related extended commands, MIDI macros and format-specific effects. `CPattern` is row/channel indexed and also carries rows-per-beat, rows-per-measure and tempo swing. The playback code separately records many tracker/version compatibility behaviours. These are execution semantics, not merely a set of note-on/note-off durations. [OpenMPT `modcommand.h`](https://github.com/OpenMPT/openmpt/blob/master/soundlib/modcommand.h) [OpenMPT `pattern.h`](https://github.com/OpenMPT/openmpt/blob/master/soundlib/pattern.h) [OpenMPT tracker-specific playback behaviours](https://github.com/OpenMPT/openmpt/blob/master/soundlib/Snd_defs.h)

The clipboard format is valuable because it already matches the tracker grid. OpenMPT documents its fixed-width cell grammar in source and shows that multi-pattern text may add order data, row counts, pattern names, signatures, and swing; the serializer emits each selected cell's note, instrument, volume command/value, and effect/parameter. That makes it a useful compact projection, but its source-format header and positional grammar do not provide the stable identities, semantic command names, or revision guard required by an AI editing API. [OpenMPT `PatternClipboard.cpp`](https://github.com/OpenMPT/openmpt/blob/master/mptrack/PatternClipboard.cpp) [OpenMPT's first-party MPTPatterns example](https://github.com/OpenMPT/MPTPatterns)

MusicXML explicitly separates notation-oriented elements from data used for sonic/MIDI realization. It permits unsupported notation and metadata through `other-notation` and `miscellaneous-field`, but the specification warns that an unstandardized `other-notation` extension does not provide application interoperability. Thus MusicXML could carry opaque tracker payloads, but doing so would recreate the native schema inside a more verbose wrapper. [MusicXML MIDI-compatible tutorial](https://www.w3.org/2021/06/musicxml40/tutorial/midi-compatible-part/) [MusicXML `other-notation`](https://www.w3.org/2021/06/musicxml40/musicxml-reference/elements/other-notation/) [MusicXML `miscellaneous-field`](https://www.w3.org/2021/06/musicxml40/musicxml-reference/elements/miscellaneous-field/)

### Required canonical envelope

Every Score Context response should contain, at minimum:

```json
{
  "schema_version": "openmpt.score-context/1",
  "project_id": "opaque-stable-id",
  "project_revision": "opaque-revision-token",
  "upstream_baseline": "pinned-build-or-source-revision",
  "module_type": "MPTM",
  "range": {
    "sequence": 0,
    "orders": [12, 13],
    "patterns": [{"pattern": 7, "rows": [16, 48], "channels": [0, 3]}]
  },
  "projection": "semantic-sparse",
  "score": {}
}
```

- Treat `project_revision` as opaque. Every proposed mutation must repeat it as `base_revision`; a mismatch returns a conflict and a fresh context locator rather than silently rebasing.
- Make the range structural (`sequence -> order -> pattern -> row range -> channel set`). The current GUI selection may choose the initial range, but it is not its durable identity.
- Include a pattern occurrence/order locator when musical meaning depends on where a reused pattern is played. Do not identify a fragment by pattern number alone.
- Use stable semantic enum names and numeric/raw parameters in canonical events. Format-specific display letters may be included as hints, never as the only effect identity.
- Include only referenced instrument/sample/plugin summaries by default and expose details through separate resources. Record omissions/truncation explicitly so the model does not interpret absent context as empty musical state.
- Preserve source facts separately from derived interpretations. Inferred note durations, sounding pitch, bars/beats, or flattened control flow must carry `derived: true` plus the derivation/options; edits target source pattern events.

The exact revision token algorithm is intentionally left to the context-contract ticket. It must change for every relevant document mutation and be cheap to compare; the AI must not construct or interpret it.

### MCP shape

- Expose immutable snapshot resources with parameterized URIs, for example `openmpt://project/{project_id}/score-context{?revision,sequence,order_start,order_end,pattern,row_start,row_end,channels,projection}` and MIME type `application/json` or a registered vendor JSON type. A URI naming an explicit revision must keep returning that snapshot while retained, or return a clear expired/not-found error; it must never drift to the latest state.
- Provide a read tool such as `get_score_context` for model-selected ranges. Return the canonical envelope in `structuredContent` when small and a resource link/embedded resource when large. MCP tool definitions support JSON Schema input/output and tool results support structured content, resource links and embedded resources. [MCP tools specification](https://modelcontextprotocol.io/specification/2025-06-18/server/tools)
- Use resource templates for range queries and optional resource subscriptions/update notifications for live project changes. MCP resources are URI-identified, can expose text or binary contents, and define templates and subscription notifications; MCP does not itself define optimistic-concurrency semantics, so `project_revision` and `base_revision` remain application-level requirements. [MCP resources specification](https://modelcontextprotocol.io/specification/2025-06-18/server/resources)
- Keep Audio Preview and an optional off-screen Piano Roll rendering as sibling resources tied to the same project revision and structural range. Neither changes the symbolic payload's role as the canonical musical representation.

### Consequences

1. The first schema must be generated from the pinned OpenMPT baseline's semantic command vocabulary and accompanied by conformance fixtures covering every special note, volume command, effect command, reused-pattern occurrence, tempo/speed change, signature/swing value and relevant compatibility mode.
2. Measure real serialized token counts on representative sparse and dense modules before choosing whether the default prompt projection is verbose objects, compact tuples, clipboard text, or a hybrid dictionary-plus-tuples form. Token efficiency here is a design inference from representation shape, not a benchmark result.
3. Round-trip fidelity tests apply to canonical JSON only. MIDI, MusicXML, inferred-duration views and clipboard-only payloads must disclose their loss profile in the response metadata.
