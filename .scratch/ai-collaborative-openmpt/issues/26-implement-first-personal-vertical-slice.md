# Implement the first personal vertical slice: Piano Roll editing with a local AI proposal loop

Parent: ../map.md
Type: spec
Status: open
Label: ready-for-agent
Blocked by: 19, 20, 21, 22, 23, 24, 25 (all resolved)

## Problem Statement

The owner composes music in an OpenMPT-derived tracker. When writing a melody or a harmony line, they must place every note cell by cell in the Tracker view, with no time-pitch visualization, and no way to hand the bounded, mechanical part of the work — drafting a harmony under an existing melody, or a melody over existing harmony — to an AI collaborator without surrendering control of the project file.

Today the OpenMPT for AI workspace has all design decisions made and the seams proven by prototypes (issues 20–25), but no implementation: there is no real MCP surface, no AI-readable Pattern snapshot, no proposal workflow in the application, and the Piano Roll prototype from issue 22 is a prototype-build acceptance, not yet a maintained part of the development branch. The owner needs one end-to-end working loop on their own machine.

## Solution

Implement the first personally useful vertical slice defined by issue 20, exactly as scoped by issues 21–25: the owner opens the MPTM test fixture, edits a single Pattern through the synchronized Tracker/Piano Roll editor, and lets one local Agent over MCP draft the complementary voice(s) through a bounded proposal loop. The owner reviews one whole immutable proposal in the synchronized Tracker/Piano Roll projections, applies or rejects it as a whole, hears it through ordinary playback, reverses it with one ordinary Undo step, gives natural-language feedback for a fresh proposal when dissatisfied, and finishes with a manual Save As plus close/reopen verification.

Success is the observable owner workflow, verified per issue 20's required-verification list, including the human verdict that at least one generated result is worth keeping.

## User Stories

1. As the owner, I want to build and debug the app with the smallest x64 Debug loop established by issue 21, so that I can iterate on this slice without CI, installers, or pinned toolchains.
2. As the owner, I want the Piano Roll pane from issue 22 carried into the main development branch, so that I can see Pattern notes on a time-pitch plane while composing.
3. As the owner, I want the Piano Roll to stay a projection of the active native `CPattern` with no second note store, so that everything I see and edit is the real Tracker data.
4. As the owner, I want unsupported tracker-only content (effects, volume commands, PC notes) shown as explicit markers rather than fabricated notes, so that I never mistake hidden data for an empty cell.
5. As the owner, I want Piano Roll selection and Tracker selection to stay synchronized through the shared `PatternRect`, so that I can work from either surface interchangeably.
6. As the owner, I want a purpose-built MPTM fixture with two resettable four-to-eight-bar starting states — melody with empty harmony voices, and harmony with an empty melody voice — so that I can test both collaboration directions repeatably.
7. As the owner, I want each target Pattern to appear exactly once in the Order list of the fixture, so that the first test never disguises shared-Pattern consequences as per-instance editing.
8. As the owner, I want one local Agent to connect to OpenMPT through an MCP client and the Sidecar, so that the AI never runs inside the UI or audio core.
9. As the Agent, I want to explicitly attach to one running app instance and one open document, so that I never edit the wrong project.
10. As the Agent, I want `get_pattern_context` to return a sparse semantic Score Context of the bound Pattern — identity, dimensions, timing context, format limits, existing instrument/sample references, and every non-empty cell with stable semantic names plus raw values — so that I can reason about the music without a C++ memory dump.
11. As the Agent, I want reads unrestricted by the write envelope but hard-bound to the one Pattern chosen at session start, so that I can inspect the whole Pattern but never touch another one.
12. As the owner, I want the first MCP surface to expose only Pattern editing mode, so that Order, instruments, samples, plugins, routing, tempo, and project setup remain human-owned.
13. As the Agent, I want `replace_pattern_segment` to write one contiguous row segment of one channel per call with complete desired cells, so that one call can add, move, replace, and delete notes in one voice.
14. As the Agent, I want multiple single-voice calls to accumulate into one private candidate and one final proposal, so that a multi-voice harmony is one reviewable unit.
15. As the Agent, I want every failed call to leave the candidate exactly as it was, with the failing row, field, value, and reason reported, so that I can correct and retry without hidden state drift.
16. As the Agent, I want the initial write envelope to be the owner's explicit `PatternRect` selection, or the whole bound Pattern when nothing is selected, so that I never write outside what the owner granted.
17. As the Agent, I want expanding an explicit envelope to pause my call for the owner's non-modal approval, with the enlarged envelope session-local afterwards, so that the owner stays in charge without repeated interruptions.
18. As the owner, I want a global ask-each-time / always-approve preference for range expansion, defaulting to ask, so that I can tune the interruption level.
19. As the owner, I want writes limited to ordinary pitched notes, empty notes, note-offs, existing instrument references, and ordinary volume values, so that everything else in the Pattern is preserved byte-for-byte.
20. As the owner, I want a write that would clobber unsupported content to fail the whole call rather than silently merge or drop it, so that no tracker semantics are ever silently approximated.
21. As the owner, I want every read and mutation call to take a short occupancy, with `occupy=true` promoting to a retained cross-call session, so that the document is never observed or changed mid-mutation.
22. As the owner, I want human project writes, Undo, and Redo blocked during retained occupancy while navigation, inspection, selection, and playback stay available, so that I can watch the Agent work without losing safety.
23. As the owner, I want retained occupancy always visible with a high-priority human release, so that I can take back control at any moment, including while a range-expansion approval is pending.
24. As the owner, I want a finite occupancy timeout (initially five minutes, tunable) that pauses during approval waits, so that a stuck Agent cannot hold my project forever.
25. As the Agent, I want `handoff_for_review` to freeze an immutable proposal and release occupancy atomically, and `abort_session` to discard the candidate and release occupancy atomically, so that there is never an unlocked mutable candidate.
26. As the Agent, I want `release_occupancy` to end a read-only retained session and fail when candidate edits exist, so that ending state is always explicit.
27. As the owner, I want a revision mismatch, occupancy loss, abort, or timeout to invalidate the whole candidate rather than rebase it, so that stale work never silently lands.
28. As the owner, I want the bound Pattern ID, base revision, and initial write range to never follow my later UI navigation or selection changes, so that the session stays pinned to what was granted.
29. As the owner, I want to review one normalized whole-proposal cell diff in synchronized Tracker and Piano Roll evidence, so that I can judge the musical change from either projection.
30. As the owner, I want review evidence limited to the affected range, target voices, note differences, and revision status — with error explanations only when a real validation problem exists — so that the UI does not drown me in defensive proofs.
31. As the owner, I want partial acceptance to be absent and to fail as unsupported if attempted, so that the musical unit is never fragmented.
32. As the owner, I want Apply to be a deliberate human action that revalidates document identity, the Pattern dependency signature, format constraints, and every final cell, then commits everything or nothing, so that Apply can never half-land.
33. As the owner, I want one successful Apply to advance the document revision exactly once and create exactly one ordinary OpenMPT Undo step, so that the AI's multi-call work collapses into one reversible human action.
34. As the owner, I want a stale proposal to be reported and never silently rebased, so that I always know the proposal no longer matches the music.
35. As the owner, I want Reject to discard the proposal with no Pattern, signature, or Undo-stack change, so that rejection is free.
36. As the owner, I want to hear the committed result through ordinary playback only after Apply — never the private candidate — so that what I hear is always real document state.
37. As the owner, I want ordinary Undo to reverse the complete applied proposal in one step, so that recovery is a native gesture.
38. As the owner, I want to describe an unsatisfactory result back to the Agent in natural language and get a fresh proposal from the current revision, so that feedback is conversational.
39. As the owner, I want to Save As, close, and reopen the test copy myself, so that persistence stays a human action and I can verify the MPTM round trip.
40. As the Agent, I want typed failure results (`notAttached`, `documentGone`, `owningThreadRequired`, `instanceGone`, occupancy loss, stale, validation failure), so that I can react to the failure layer rather than parse prose.
41. As the owner, I want the app-side IPC broker to only validate and queue envelopes while the document owning thread alone captures Pattern snapshots, so that the thread-safety seam from issue 24 holds in the real implementation.
42. As the owner, I want the AI/MCP settings surface to hold MCP enablement, service status, occupancy timeout, and the expansion-approval preference in one focused area, so that the feature is discoverable without redesigning OpenMPT settings.
43. As the owner, I want both melody-to-harmony and harmony-to-melody workflows to pass through the same MCP and proposal loop, so that the slice proves the collaboration pattern rather than one lucky direction.
44. As the owner, I want at least one generated result good enough to keep, so that the slice is personally useful rather than a tech demo.

## Implementation Decisions

Modules to build or modify, per the resolved map:

- **Application Capability seam (new, protocol-neutral)**: a narrow facade above `CModDoc` / `CSoundFile`, introduced capability-by-capability per issue 05 — only the five Pattern-mode tools from issue 23: `get_pattern_context`, `replace_pattern_segment`, `handoff_for_review`, `abort_session`, `release_occupancy`. No capability catalogue, no other domains.
- **Score Context serializer (new)**: sparse semantic JSON per issue 04 and 23 — revision-bound, per-issue-23 opaque session token instead of an Agent-visible revision number; clipboard text, MIDI, and MusicXML remain non-canonical projections and are not part of this surface.
- **Session, occupancy, and candidate state (new, app side)**: the state model accepted in issues 20/23/25 — short per-call occupancy, `occupy=true` promotion, retained cross-call session with token, private candidate, five-minute renewable timeout paused during approval waits, high-priority human release, single active session on one document.
- **Pattern dependency signature (new, internal)**: opaque stale signature over the bound Pattern plus timing, module-format, and referenced instrument/sample state, per issue 23. Never exposed to the Agent; Apply compares against it and reports stale instead of rebasing.
- **Proposal review and Apply (new UI, human-only actions)**: whole-proposal normalized diff with synchronized Tracker/Piano Roll evidence, whole Accept/Reject, atomic commit with complete prevalidation, one revision increment, one ordinary Pattern Undo step via `CPatternUndo`. No partial acceptance, no AI Apply/Reject/Undo/Save tools.
- **IPC broker and owning-thread dispatch (new, app side)**: validate-and-queue only, per issue 24; the document owning thread alone rechecks document liveness and captures the immutable Pattern snapshot. First-slice transport is the local Sidecar over a current-user Windows named pipe per issue 06, minus the mature machinery.
- **MCP Sidecar (new process)**: stateless, client-launched over stdio, translator-only between MCP tools and the protocol-neutral app envelope; explicit app and document attachment; no document guessing, no music semantics, no state ownership, per issues 06/24.
- **Piano Roll pane (carried into main development line)**: the issue-22 prototype pane — child HWND with private bitmap paint, `WS_CLIPCHILDREN`, exclusion from the Tracker `ScrollWindow` region, whole-Pattern pitch range, keyboard/lanes/time ruler, tracker-detail markers — merged and maintained as the human editor half of this slice. `Draw_pat.cpp` / `View_pat.cpp` integration follows the prototype's proven seams.
- **AI/MCP settings area (small addition)**: enablement, service status, occupancy timeout, ask/always-approve expansion preference.
- **MPTM test fixture (new)**: the two resettable starting states with single-occurrence target Patterns, per issue 20.
- **Reuse, not rewrite**: native `CPattern` / `ModCommand` storage, module-format limit checks, `CPatternUndo` snapshots, `PatternRect` selection, playback, and format I/O are starting materials per issues 02/12/17-direction — wrapped, not duplicated.
- **First-slice deferrals from issues 06/20/23 remain deferred**: multi-instance discovery, leases with heartbeats, Operation Receipts, session sequence numbers, capability events, fault-injection hardening, offline render, Mini Audio Reviewer, Context Packages, Capability Modes beyond this one implicit Pattern mode, and the second-Agent contract are not built now.

## Testing Decisions

A good test here exercises **external behavior at the module seam**, not implementation internals: call the capability facade the way the Sidecar does, drive the review UI the way the owner does, and assert on document state, revision counts, Undo behavior, and typed failures — never on private fields or call sequences inside modules.

Seams (highest possible, one per module boundary):

- **Capability facade seam** — the five Pattern-mode tools are tested directly in-process against a real `CModDoc` with the fixture loaded: Score Context contents, segment validation, all-or-nothing failures with candidate unchanged, unsupported-content preservation, envelope and expansion-approval behavior, occupancy promotion/release/timeout, handoff/abort atomicity, dependency-signature staleness. Prior art: the existing `test/` suite (`mpt_tests_*` files) already exercises `CSoundFile`/Pattern behavior in-process with fixtures like `test.mptm`.
- **Proposal-review-Apply seam** — driven through the proposal model the UI binds to: whole Accept/Reject, one revision increment, one Undo step, stale rejection, commit-failure rollback, Reject leaving zero trace. Mirrors the six guided cases of the accepted issue-25 logic prototype, now against real document state.
- **Sidecar translation seam** — Sidecar tested as a process over its stdio boundary against a scripted app-endpoint double: MCP-to-envelope translation, explicit attachment, `notAttached`/`documentGone`/`owningThreadRequired`/`instanceGone` mapping, per issue 24's four guided cases. The app broker is covered from the facade seam; only translation correctness needs the process boundary.
- **End-to-end owner verification** — the manual checklist from issue 20: both collaboration directions, selection/no-selection scopes, multi-call accumulation, occupancy behaviors, failed-call and invalidation cases, atomic Apply/Undo, playback, feedback retry, Save As/reopen, and the human keep-verdict. This stays manual; no GUI automation substitutes for owner review, per the issue-22 precedent.

New test files follow the existing `test/mpt_tests_*.cpp` registration pattern where they test in-process seams; Sidecar tests use whatever minimal harness the Sidecar's own toolchain adopts, with no new shared test framework introduced.

## Out of Scope

Per the map and issue 20's explicit absences:

- Order AI/UI work, cross-Pattern operations, Pattern creation/deletion/resize/rename, full-song autonomous composition.
- Instrument, sample, plugin, routing, tempo, meter, or project-setup reads-beyond-summaries or writes of any kind.
- Effect commands, note cuts, complex delays, PC/PCS notes, fades, and non-volume volume-column writes (preserved, never written).
- Partial proposal acceptance, mandatory pre-Apply audition, impact tiers, Mini Audio Reviewer integration, Audio Preview / Context Package machinery, Piano Roll rendering as AI evidence.
- Mature lease, Operation Receipts, multi-client or remote MCP operation, multi-instance discovery, capability events, fault-injection hardening.
- Piano Roll Focus layout (issue 19's accepted design is a later ticket), note insertion/resizing in the Piano Roll, auxiliary lanes, production visual polish.
- Release packaging, CI, reproducible builds, public-product readiness, upstream contribution.
- AI playback, render, save, Apply, Reject, Undo, or Redo tools.

## Further Notes

- This spec consolidates resolved issues 19–25; their tickets remain the primary record. Where wording differs, the tickets win.
- The issue-22 Piano Roll acceptance applies to the reviewed executable on branch `prototype/synchronized-piano-roll-editing-slice` (commit `18e3156f`); merging it forward is part of this work, not proof the work is done.
- The issue-24 and issue-25 verdicts are logic-verdicts on in-memory prototypes; they establish state boundaries and failure shapes only. This spec's end-to-end verification is the first real evidence for transport, occupancy, serialization, and atomic commit.
- Deferred items graduate only through observed need in real use, per the map's "Not yet specified" list — none of them may be pulled into this slice to "finish the design."
