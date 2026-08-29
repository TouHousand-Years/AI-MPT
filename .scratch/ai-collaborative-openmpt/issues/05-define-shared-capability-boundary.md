# Define the shared Application Capability boundary

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by:

## Question

Which stable query, edit, and operational capabilities should the first shared application layer expose; how should they be grouped and versioned; and which legacy OpenMPT actions must initially remain UI-only because they cannot yet satisfy transaction, undo, safety, or realtime constraints?

## Comments

### Implementation scope correction — 2026-08-29

The capability boundary remains a design direction, not a mandate to migrate
OpenMPT wholesale before useful work begins. The current map introduces only
the Pattern queries and edits required by the active vertical slice. Broader
base, Order, audition, plugin, and project capabilities graduate after real use
shows that they are needed.

The design was grilled in rounds and confirmed by the human collaborator on 2026-08-28. The capability priority below uses workflow centrality inferred from the official OpenMPT manuals, default command surface, and the pinned-source inventory in issue 02; OpenMPT publishes no user-operation telemetry, so it is not a claim about measured click frequency. Issue 08 later refines proposal review: Agent operations before handoff affect a private candidate state, and only the human-approved accepted subset becomes one project commit.

## Answer

### Boundary and contract

Add one narrow, protocol-neutral Application Capability facade above `CModDoc` / `CSoundFile`. Callers see one consistent entry point and capability manifest; internally, handlers and plain data contracts are separated by musical domain. Do not export MFC command identifiers, view handlers, modal workflows, mutable model references, or GUI automation.

The facade contract requires:

- owning-thread execution and no edit work on the realtime audio callback;
- immutable, revision-bound query results;
- complete prevalidation against the current module format;
- `expectedRevision` on edits;
- all-or-nothing application, one undo step, and unchanged state on failure;
- stable typed errors and one post-commit capability event;
- a protocol major version plus a capability manifest that reports each available capability and schema version.

Legacy functionality migrates into this layer only after it can meet that contract. The first implementation proves the invariants with bounded Pattern queries and edits, then expands to the closed-loop composition boundary below.

**Pattern storage, format-limit validation, supported Pattern transforms, domain-local undo/redo, playback, and offline audio rendering reuse the original OpenMPT implementation.** The new facade wraps those implementations with immutable data contracts, revision checks, transaction coordination, typed failures, and events. Reuse of a legacy implementation does not waive any facade invariant or imply that an existing MFC handler is itself a shared capability.

### First shared capability set

The first release targets the actual composition loop rather than broad menu coverage.

| Area | First shared capabilities | Boundary |
|---|---|---|
| Project context | Read project identity, format limits, revision, sequences, Pattern metadata, current structure, existing channel / Sample / Instrument / Plugin summaries, and transient playback status | Read-only; return stable plain data rather than live objects |
| Pattern composition | Read bounded Pattern ranges; batch replace or clear cells; copy or move phrases within supported scopes; transpose, change instrument, interpolate, amplify or fade, and stretch or shrink supported selections | All edits are format-validated, revision-bound, proposal-compatible, and undoable |
| Order arrangement | Read song structure; create, duplicate, rename, and resize Patterns; perform simple Order insertion, deletion, and movement | Enable each mutation only after sequence and cross-domain transaction support exists |
| Existing sounds | Refer to already available instruments or samples from Pattern data | First release does not create, import, or deeply edit sound sources |
| Audition and review | Assemble revision-bound Score Context with a bounded audio file, an AI-oriented Piano Roll image, and a Mini Audio Reviewer opinion | AI rendering is offline and independent of the live player; issue 07 defines the all-or-nothing Package contract |
| Human audition | Continue reading, navigating, and auditioning while an Agent Edit Session is active | Human project writes remain frozen during the session |

Initial tempo, speed, meter, rows-per-beat, and similar project setup are human-owned. Agents may read them but do not modify them in the first release. Saving is likewise performed by the human and the application's automatic persistence; no AI save tool is exposed.

### Layered tool presentation and Skills

Do not present the full OpenMPT command surface to an Agent at once. Keep a small base query layer available and expose other schemas through explicit Capability Modes:

1. **Base context** — project identity, revision, structure navigation, available sounds, format limits, and status.
2. **Pattern composition** — Pattern reads, batch cell edits, phrase operations, and semantic transforms.
3. **Order arrangement** — Pattern lifecycle and Order structure operations.
4. **Audition and review** — bounded audio and Piano Roll rendering, temporary review resources, and Mini Audio Reviewer orchestration.

Later Sample, Instrument, and Plugin capability families receive their own modes rather than expanding the initial modes indefinitely.

Each mode has a focused Skill that teaches tracker semantics, safe tool composition, limitations, and examples. Entering a mode loads that Skill and only the relevant tool schemas. Skills contain stable guidance; live format limits and per-project availability come from the capability manifest and remain programmatically enforced.

Capability Modes are a presentation boundary, not a transaction boundary. An Agent may accumulate Order and Pattern operations into one Change Proposal by moving between modes. The proposal becomes committable only when every included operation is supported by one atomic cross-domain transaction and undo step. Entering a mode does not change the human UI selection, focus, playback position, or layout.

### Main Agent and Mini Audio Reviewer

The Main Agent owns composition decisions and all proposed edits. Its first-release context comprises:

- canonical symbolic Score Context;
- a Mini Audio Reviewer natural-language opinion from every completed Context Package;
- the companion Piano Roll rendering when the user's Main Agent vision preference is enabled.

The raw Audio Preview is not sent to the Main Agent in the first release. Supporting direct native-audio reasoning by a future Main Agent is not an initial requirement.

For every Score Context Package, the application renders the requested range to a bounded file instead of driving the live OpenMPT player and creates an AI-oriented Piano Roll from the same immutable snapshot. The Mini Audio Reviewer must support text, image, and audio input and receives both artifacts together with the selected musical fragment, range metadata, its system prompt, and an optional message from the Main Agent. The Piano Roll is always available to the Reviewer even when the Main Agent vision preference is disabled.

The reviewer can only advise. Its response is treated as untrusted natural-language listening commentary because the smaller audio model cannot be assumed to produce reliable structured output. It has no Application Capability that can alter the project. Score Context, Piano Roll rendering, Audio Preview, and the opinion provenance must identify the same project revision and musical range.

### Agent Edit Session and human concurrency

An Agent Edit Session begins before the Main Agent's first candidate mutation and lasts through its edit-render-review loop until completion or a human-review checkpoint. During it:

- human project writes, undo, and redo are frozen;
- human queries, navigation, visual inspection, and audition remain available;
- AI audition uses isolated offline rendering and does not depend on live playback position;
- the UI shows what the Agent is editing or reviewing and always offers cancel;
- the session has a timeout so model or network failure cannot retain edit authority indefinitely.

Candidate operations affect a private proposal state rather than the document revision or undo history. Cancellation discards that state. At the human-review checkpoint, Agent edit authority ends; issue 08 defines inspection, partial acceptance, and audition of the candidate. A deliberate human apply reacquires edit authority and either commits the exact accepted subset fully as one revision and undo step or leaves the document unchanged. The Agent Edit Session is project-wide because a single proposal may span Pattern and Order domains.

### Initially human-only capability families

The following remain available through the existing human UI but outside the first Agent surface:

- Sample import, slot topology, waveform DSP, external-sample relinking, and file export;
- deep Instrument maps, envelopes, NNA / DCT / DNA, tuning, and cross-Sample edits;
- Plugin discovery, loading, removal, routing, native editors, arbitrary parameter access, and scanning;
- arbitrary-path save-as, formal export, format conversion, cleanup, append-module, and global device or application settings;
- any compound legacy action that cannot yet prevalidate completely, apply atomically, undo reliably, avoid realtime interference, and obey file / plugin / time / memory limits.

This is a migration gate, not a permanent product limit. Pattern-stored Plugin automation remains eligible as Pattern data when it satisfies the Pattern transaction contract.
