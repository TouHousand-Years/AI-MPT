# 27: Piano Roll pane into the main development line

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The issue-22-reviewed Piano Roll pane — child HWND with private bitmap paint, `WS_CLIPCHILDREN`, exclusion from the Tracker pixel-scroll region, whole-Pattern pitch range, piano keyboard / pitch lanes / time ruler, channel-coloured notes with strong selected-note treatment, and explicit tracker-detail markers for unsupported content — is merged from the prototype branch into the main development line and works in the standard x64 Debug build. It stays a projection of the active native `CPattern` over the shared `PatternRect`, with one synchronized playhead and ordinary Pattern Undo.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [ ] In the mainline x64 Debug build, scrolling the Tracker up and down at least 20 times leaves the Piano Roll title, keyboard, grid, notes, legend, and state line single and intact with no repaint trails or flicker.
- [ ] Selecting a note from either surface makes the other surface follow through the shared `PatternRect`; transposing one note (drag or `+1` button) and undoing through ordinary Pattern undo returns both surfaces to the same note.
- [ ] Normal Pattern playback shows one synchronized playhead on both surfaces; stopping playback clears the playhead without residue.
- [ ] In `test.mptm`, PC/effect/volume-only content appears as explicit tracker-detail markers, never as fabricated pitched notes.
- [ ] The pane remains a projection of the active native `CPattern`: no second note store, no persistence format, no parallel undo history; availability still gated by the minimum Pattern-view client size from the prototype.

## Implementation notes (agent, 2026-09-07)

The pane itself was merged earlier (commit `18e3156f`); the native capability seam, broker/panel, occupancy gating, and real-process integration tests landed in `5270571e5`–`6d430551f`. Automated verification: native capability suite PASS (`build-local.ps1 -Test`), real-process two-direction integration test OK over the actual sidecar and named pipe, sidecar translation suite OK. The five checklist items above remain owner-manual demo items and are intentionally unchecked.

Code review (standards + spec) findings fixed in `27-4`: `AICommandNames.h` is now self-contained inside `namespace AI` (was included through a namespace in `AIPattern.cpp`); the duplicated AI command gate in `CModControlDlg`/`CModScrollView::WindowProc` extracted to one helper; review-evidence lines no longer append raw JSON dumps (issue 26 story 30); the panel raises its z-order without stealing focus while occupancy or a proposal is active (story 23); null `OPENMPT_AI_TEST_REPORT` env guarded; transient build/test artifacts untracked via `.gitignore`.

Deliberate interpretations, recorded so the owner can veto them at the demo:

- **Short per-call occupancy** (story 21): reads are revision-bound atomic snapshots; writes require an explicit `occupy=true` retained session, because candidate edits need a session token to reach `handoff_for_review`. A sessionless write returns `occupancyLost` instead of auto-creating one.
- **Single-cell selection** (story 16): the tracker cannot distinguish a 1×1 selection from a placed cursor, so `AISelection()` treats cursor-only as "nothing selected" and grants the whole Pattern as envelope.
- **Calls during a pending expansion approval**: the single-connection broker serializes frames, so further agent calls wait (not fail) until the owner decides or the occupancy timeout fires. Typed-failure-on-queue would require transport rework deferred by issue 06.
- **MCP enablement defaults to on** for the personal slice; the pipe is local-only with a current-user ACL.
- **Undo.cpp eviction reorder** is spec-driven, not scope creep: history is only evicted after a fully successful buffer preparation (commit-failure rollback, story 32) and `CPatternUndo::Undo` is blocked during occupancy (story 22).
- The `DrawEvidence` three-panel note plot is in-panel human review evidence (story 29); "Piano Roll rendering as AI evidence" remains out of scope.
