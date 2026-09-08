# 32: Proposal review UI and atomic Apply in the app

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The human-facing proposal review surface from issue 25, built on the issue-29 facade and drivable in-process (test entry or debug command) before any transport exists: one normalized whole-proposal cell diff shown through synchronized Tracker and Piano Roll evidence (baseline / proposal / current-document states), the proposal's revision status, and whole-proposal Accept (Apply) and Reject actions only. Apply is a deliberate human action that reacquires edit authority, rechecks the Pattern dependency signature, revalidates every final cell and format constraint, then commits the complete diff or nothing — one revision increment, exactly one ordinary `CPatternUndo` step even for multi-voice proposals. Stale proposals are reported and never rebased; Reject leaves Pattern cells, dependency signature, and Undo stack untouched; partial acceptance is absent and fails as unsupported if attempted. Retained-occupancy UI is always visible with the high-priority human release. Also includes the focused AI/MCP settings area: MCP enablement, service status, occupancy timeout, and the ask-each-time / always-approve range-expansion preference (default: ask).

**Blocked by:** 29 (Pattern capability seam with the five Pattern-mode tools)

**Status:** resolved

## Acceptance criteria (demo to owner, against real document state)

- [x] With an injected proposal, the owner sees the normalized whole-proposal diff in synchronized Tracker and Piano Roll evidence; before Apply the current document equals the baseline.
- [x] Apply of a multi-voice proposal advances the document revision exactly once, creates exactly one ordinary Pattern Undo step, plays back through normal playback, and one ordinary Undo restores the complete pre-Apply Pattern (the voice calls are not undone separately).
- [x] Reject discards the proposal with zero change to Pattern cells, dependency signature, and Undo stack.
- [x] After a post-handoff human Pattern edit, Apply of the old proposal is refused as stale (never silently rebased); the stale proposal remains available read-only for comparison.
- [x] A simulated commit failure leaves every Pattern cell, the dependency signature, and the Undo stack unchanged, with the same immutable proposal still available for retry; attempted partial acceptance fails as unsupported and changes no state.
- [x] During retained occupancy the owner sees the occupancy indicator, can navigate/inspect/play but cannot write/Undo/Redo, and can force-release at any time (including while a range-expansion approval is pending), invalidating token and candidate.
- [x] The focused settings area exposes MCP enablement, service status, occupancy timeout, and the expansion-approval preference; changing the preference takes effect for subsequent expansion requests.

## Work log — 2026-09-08

Issue 32 was completed by verifying and closing the proposal-review surface
that had accumulated across tickets 27–31. The application panel already binds
to the public `PatternCapability` proposal model and exposes whole-proposal
Apply/Reject only, a normalized Tracker cell list, Baseline / Proposal /
Current-document Piano Roll evidence, visible retained-occupancy and proposal
status, the high-priority release action, expansion approval controls, and the
focused MCP settings row. The confirmed test seam remains the public proposal
model; owner-facing UI and playback verification remain manual as specified by
the parent ticket.

The fixture-driven native regression suite now additionally proves that:

- handoff exposes one current normalized multi-voice diff while every live cell
  still equals its baseline and the Document Revision is unchanged;
- unsupported partial acceptance and simulated commit failure preserve the
  exact immutable proposal, Pattern, revision, and native Undo state for retry;
- a stale proposal retains its original diff read-only and is never rebased;
- force-release during a pending expansion invalidates both the request and
  token, discards the candidate, and changes no document or Undo state; and
- switching from ask-each-time to always-approve affects the next expansion
  request in the same Agent Edit Session.

Verification:

- `.\build-local.ps1 -Test` — PASS, including the two-voice proposal report.
- `python test-fixtures\validate_fixture.py` — all 131 checks passed, including
  playback evidence.
- `python -m unittest discover -s sidecar -v` — 25 tests passed, with the three
  opt-in native integration tests skipped.
- `libopenmpt_test.exe` — completed with only the two known host-locale
  transcode failures at `tests_string_transcode.hpp` lines 138 and 248; no
  Pattern capability or proposal-review regression was reported.
