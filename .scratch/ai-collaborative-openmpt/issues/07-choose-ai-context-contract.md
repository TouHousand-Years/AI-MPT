# Choose the AI Score Context and Audio Preview contract

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by:

## Question

What canonical, revision-bound Score Context should the Main Agent receive for a project, selection, or requested musical range; when should it be accompanied by a Mini Audio Reviewer opinion and optional off-screen Piano Roll rendering; and how should fidelity, token size, tracker-specific events, shared-range provenance, and stale-context detection be represented?

## Comments

### Implementation scope correction — 2026-08-29

Revision-bound semantic Score Context remains authoritative, but the first AI
read loop does not require a complete all-or-nothing multimodal Context Package.
Audio Preview, AI-oriented Piano Roll evidence, and the Mini Audio Reviewer are
deferred enrichments whose value will be judged after the symbolic round trip
works locally.

The design was grilled in rounds and confirmed by the human collaborator on 2026-08-28. It turns the canonical representation chosen in issue 04 into a bounded context-assembly contract and supersedes issue 05 only where that issue described audition and Piano Roll evidence as request-dependent.

## Answer

### Context Package and acquisition layers

A **Context Package** is the only complete Score Context delivery unit. It binds one immutable document snapshot and one normalized Musical Range to:

- the canonical Score Context;
- one bounded Audio Preview;
- one AI-oriented Piano Roll rendering;
- one Mini Audio Reviewer opinion produced from the Score Context, Audio Preview, and Piano Roll.

**Audio playback, offline mixing/rendering, and stream encoding reuse the original OpenMPT implementation.** The project extracts those functions from the existing UI workflow behind a bounded, cancellable, snapshot-bound adapter. Context Package identity, range/revision binding, resource handles, caching, provenance, Piano Roll rendering, and Reviewer orchestration are new downstream functionality.

The four required components are all-or-nothing. Package creation is an asynchronous job: its initial result is only a job identifier and status, not a usable Score Context or partial Package. The final job either publishes a complete Package or fails. A render, image, reviewer, network, deadline, authorization, or resource failure never degrades into a Score Context-only success. Piano Roll generation is mandatory for the Reviewer; a visible application-level preference with an optional per-project override controls only whether the image is also exposed to the Main Agent.

Lightweight queries for project identity, revision, structure navigation, format limits, available-sound summaries, and resource status do not create a Context Package and therefore do not invoke rendering or an external model. Context acquisition is layered:

1. obtain a lightweight project overview;
2. create a Package for one explicit working range;
3. expand referenced instrument, Sample, Plugin, adjacent-section, or other Pattern details on demand.

The default is not a whole-project Package. Whole-project context is allowed only when explicitly requested and within every bounded resource and playback limit.

### Identity, snapshot, and Musical Range

The external runtime identity is `(appInstanceId, documentId)`. Every Package records those values, an unguessable `contextPackageId`, and the captured document `revision`. Internally the revision is monotonically increasing; externally it is an opaque string that clients may only compare and return unchanged. Mutation contracts use `expectedRevision`, and a Change Proposal also cites the Package that informed it for provenance. Only a complete, identity-matching, current Package may support a mutation.

`latest` and `currentSelection` are convenience inputs, not durable locators. On the document owning thread, the app atomically captures document identity, revision, GUI selection where requested, and the normalized range. The resolved Package never follows later selection or project changes. An inconsistent capture returns `revisionConflict` rather than silently retrying against different music.

A normalized Musical Range has a scope and one ordered segment describing:

- sequence identity;
- order occurrence and resolved Pattern identity;
- a half-open row interval `[start, end)`;
- a sorted, duplicate-free set of focus channels.

The first release permits one continuous playback span per Package. Multiple disjoint fragments require separate Packages. Empty, ambiguous, invalid, or out-of-bounds ranges fail explicitly. A whole-project request is resolved into the same concrete structural form rather than retained as a drifting alias. Reused Patterns always retain their order occurrence.

The Package distinguishes its editable `sourceRange`, selected `focusChannels`, and a finite `playbackTrace`. The trace records the actual order/Pattern/row occurrences produced by Pattern breaks, jumps, loops, delays, and related tracker control flow without pretending that a flattened trace is editable source data. A trace that cannot terminate within the 60-second render bound fails at the responsible control-flow location; it is never arbitrarily truncated.

Audio uses the same temporal span as the Score Context but renders the complete audible mix. Selected channels are the focus rather than a destructive solo, because routing, cross-channel effects, and musical relationships can change under soloing. The Package records `renderSettings.channelMode: fullMix` and summarizes sounding channels that are not expanded in the focus Score Context.

Every fragment carries the minimum interpretation closure needed to understand it: module format and compatibility modes, entry tempo and speed, meter, swing, effect-memory and control-flow predecessor state, and summaries of referenced sounds. This material is `interpretationContext`, not an editable event. If the closure cannot be computed reliably, Package creation fails.

### Canonical fidelity and size

The canonical model remains the versioned sparse OpenMPT semantic JSON chosen in issue 04. Each Pattern cell event records its stable semantic name, raw numeric value or parameter, source coordinate, and applicable module format. Special notes, volume commands, effect commands, and Plugin-control notes use discriminated structures. Display letters are hints only. A baseline-valid value not yet understood is preserved losslessly as `unknownRaw` rather than discarded or guessed.

Every section reports `completeness` as `complete`, `omittedByScope`, `unavailable`, or `unsupported`, with a reason and an expansion locator where possible. Absence is never allowed to masquerade as empty music. Derived values record `derived: true`, derivation and option versions, and source references. Only source events may become mutation targets; summaries, inferred durations, clipboard text, MIDI, MusicXML, and other derived projections are non-authoritative and disclose their loss profile.

Define one normative semantic model with two lossless encodings:

- `semantic-object`, optimized for readability and schema inspection;
- `semantic-compact`, using a dictionary and compact tuples.

Both must decode to the same normalized semantic model. `semanticHash` is computed from that model under schema-major canonicalization rules; each transmitted artifact also has a `contentHash` over its actual bytes. Cross-encoding fixtures must prove semantic-hash equality.

Do not expose tokenizer-specific counting, estimates, or a token benchmark. The service reports only deterministic event counts, encoded byte counts, and resource sizes. AI clients own model-context-window planning. Canonical content is never silently truncated: large results use a complete manifest plus deterministic resource chunks, and content that still exceeds negotiated hard limits fails with `resourceLimitExceeded` and a suggested narrower range. Summaries and lossy projections are separately named and cannot satisfy a mutation precondition.

### Mandatory multimodal review

The off-screen Piano Roll is an AI evidence view, not a screenshot of the human editor and not a second music model. It primarily renders the piano-roll plane itself so pitch-time geometry, note extent, overlap, density, contour, focus, padding, and useful channel or instrument grouping can compensate for weaknesses in textual and audio-only time-frequency reasoning. It omits toolbars, editing controls, parameter panels, effect-command text, and metadata already represented precisely in the Score Context.

The image uses deterministic, bounded rendering. Its provenance records focus and rendered ranges, viewport, pixel dimensions, theme, time and pitch scales, padding, cropping, generator version, and content hash. Limited contextual padding is permitted, but focus is visually distinguished and padding events never appear editable. Cropping is explicit; the renderer does not shrink the view into illegibility. A valid range containing no notes produces a verifiable empty-grid view rather than unrelated content.

The Mini Audio Reviewer must have declared and integration-tested text, image, and audio input support. There is no audio-only fallback: a capability mismatch fails with `reviewerCapabilityMismatch`. The Reviewer receives the bounded Score Context and interpretation closure, the complete-mix Audio Preview, the Piano Roll, its system prompt, and an optional Main Agent message. It has no edit capabilities.

The Reviewer returns bounded natural-language listening commentary, not structured musical truth. A trusted envelope records its model and provider, prompt version, input hashes, revision and range, duration, and completion state. Claims in the commentary never override canonical source facts. The raw Audio Preview is not exposed to the Main Agent in the first release; the Piano Roll is exposed to it only when the user-facing vision preference is enabled.

Before external review is enabled, the user explicitly selects a provider and authorizes outbound data. The UI keeps that choice visible and revocable. Only the bounded audio, Piano Roll, necessary anonymized Score Context, and optional prompt are sent; project paths, user names, unrelated resources, credentials, and diagnostic logs are excluded. Missing authorization, credentials, network access, or multimodal capability fails the whole Package.

### Job lifecycle, limits, and failures

Per document, at most one heavyweight Package job runs at a time. Concurrent requests with identical document identity, revision, normalized range, schemas, and render/reviewer options coalesce onto that job. A different request receives `contextBusy` immediately rather than entering an unbounded queue. Ordinary lightweight reads remain available.

Default stage limits are 60 seconds for audio rendering, 15 seconds for Piano Roll rendering, 120 seconds for review, and 180 seconds overall. Client deadlines and app-advertised limits may be tighter but cannot widen hard limits. A Package job does not extend an Agent Edit Session.

There is no hidden automatic retry. A failed job retains only structured diagnostic metadata and hashes. An explicit retry creates a new job linked to the previous failure; it may reuse a verified, exact-key cached artifact, but review must newly succeed before the new Package completes.

Stable context errors add `contextAssemblyFailed`, `contextBusy`, `snapshotExpired`, `renderFailed`, `reviewFailed`, `reviewerCapabilityMismatch`, and `projectionUnavailable` to the issue 06 error vocabulary. They retain a safe underlying cause such as `deadlineExceeded`, `resourceLimitExceeded`, `cancelled`, or `revisionConflict`. No failed job exposes partial musical payload as a successful Package.

Cancellation stops all safely cancellable stages and prevents Package publication. Job-exclusive Score Context, audio, image, and temporary resources are released immediately. Shared cache entries are reference-counted and remain available to other Packages. Failure diagnostics never retain note payload, image, audio, paths, prompts, or external credentials.

### Provenance, caching, retention, and staleness

The Package manifest records its identity, document identity, revision, normalized range, schema versions, creation time, terminal status, and each required artifact's handle, status, schema, byte size, hashes, and provenance. Every artifact is generated from the same immutable revision and normalized temporal range. Different Packages may share an artifact only when document identity, revision, normalized range, interpretation closure, schema major, generator and model versions, options, and hashes all match.

By default, completed Packages remain available until 30 minutes after last access under a 512 MiB per-document LRU quota. Packages actively used by an Agent Edit Session are pinned within the existing session resource quota. Only completed, unreferenced entries may be evicted. Document close and app restart invalidate every Package.

If the document changes while assembly is running, the job continues against its captured immutable snapshot. A completed old Package returns `currentRevision` and `isCurrent: false`; it remains valid for read-only analysis or comparison but not mutation. Any document revision change makes a Package stale even if the changed data lies outside its focus range. Events provide invalidation and a locator for obtaining new context; they do not mutate or automatically rebuild an existing Package.

Resource handles are opaque and unguessable but are not bearer authorization. Reading requires the same-user boundary plus a current attachment to the matching app instance and document. Reconnection requires explicit reattachment. Expired, evicted, closed-document, or prior-app handles return `snapshotExpired` and never resolve to latest state.

### Context-specific capability surface

Issue 07 defines only the context sub-contract, not the complete MCP catalogue. Its minimum operations are:

- `resolveMusicalRange`;
- `createContextPackage`;
- `getContextPackageStatus`;
- `readContextArtifact`;
- `releaseContextPackage`.

Score Context, Musical Range, Context Package, audio-review provenance, and Piano Roll metadata have independently versioned schemas. The Package lists every actual schema version. Transport protocol versions govern framing, not musical semantics. Breaking changes increment a schema major; backward-compatible optional additions increment a minor. Unknown required fields or schema majors fail as established in issue 06.

Small control responses may use MCP structured content. Complete Package artifacts use immutable resource handles and bounded chunk reads as needed. Explicit revision or Package locators either return the retained snapshot or a typed expiry error; they never drift.

### Verification requirements

Implementation is incomplete until conformance, integration, failure-injection, privacy, and load tests cover:

- every baseline special note, volume command, effect command, Plugin-control event, and preserved unknown raw value;
- reused Pattern occurrences, range normalization, GUI-selection capture, half-open boundaries, empty and invalid ranges, and control-flow loops;
- tempo, speed, signature, swing, compatibility, effect-memory, referenced-sound, and control-flow interpretation closure;
- `semantic-object` / `semantic-compact` lossless equivalence and semantic-hash equality;
- deterministic chunking without truncation and explicit hard-limit failure;
- Score Context, audio, image, and review identity, revision, range, option, and hash alignment;
- complete-mix focus semantics, AI-oriented Piano Roll geometry, padding, cropping, and deterministic rendering;
- user authorization, outbound-data minimization, credential isolation, and log redaction;
- rejection of non-multimodal Reviewers and untrusted treatment of natural-language opinions;
- all-or-nothing behavior under render, image, network, model, deadline, resource, cancellation, and app-shutdown failures;
- identical-job coalescing, competing-job rejection, cache-key isolation, reference-counted cleanup, retention, LRU eviction, and snapshot expiry;
- document edits during assembly, stale read-only access, mutation rejection, event invalidation, app restart, reattachment, and resource authorization;
- sustained context generation without blocking the document owning thread, UI, or realtime audio path.
