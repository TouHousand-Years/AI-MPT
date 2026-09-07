# 29: Pattern capability seam with the five Pattern-mode tools

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The protocol-neutral Application Capability facade above `CModDoc` / `CSoundFile` carrying exactly the five Pattern-mode tools from issue 23, callable in-process: `get_pattern_context` (immutable sparse semantic Score Context of the bound Pattern — identity, dimensions, timing context, module-format limits, read-only instrument/sample summaries, every non-empty cell with stable semantic names plus raw values), `replace_pattern_segment` (one contiguous row segment of one channel per call into a private candidate; complete desired cells; all-or-nothing validation; unsupported tracker semantics preserved byte-for-byte or the whole call fails), `handoff_for_review` (freeze immutable proposal + release occupancy atomically), `abort_session`, and `release_occupancy`. Includes session binding (Pattern, baseline, initial write envelope from explicit `PatternRect` or whole Pattern), short per-call occupancy with `occupy=true` promotion to retained cross-call occupancy, the five-minute renewable timeout paused during expansion-approval waits, human force-release, the internal Pattern dependency signature with stale detection, and write-envelope expansion with the ask/always-approve preference hook. Blocked human writes/Undo/Redo during retained occupancy; reads never restricted by the write envelope but never cross the bound Pattern.

**Blocked by:** 28 (MPTM collaboration fixture)

**Status:** resolved

## Acceptance criteria (demo to owner)

- [x] Test suite (following the existing `mpt_tests_*` pattern, run against the issue-28 fixture) is green covering: Score Context contents and narrowing, single-channel contiguous segment writes, multi-call accumulation into one candidate and one normalized final diff, all-or-nothing failure with candidate exactly unchanged and typed row/field/value/reason feedback, and byte-for-byte preservation of unsupported content (including whole-call failure when a requested note would clobber a special event).
- [x] Occupancy behavior verified by test: short occupancy per call, promotion via `occupy=true`, retention across calls, omission not releasing promotion, explicit handoff/abort/release endings, `release_occupancy` failing while candidate edits exist, timeout expiry, and human force-release invalidating token and candidate.
- [x] Dependency-signature behavior verified by test: changes to the bound Pattern or its dependencies invalidate session/proposal (stale, never rebased); changes to an unrelated Pattern do not.
- [x] Write-envelope behavior verified by test: explicit selection and whole-Pattern scopes, envelope expansion gated on approval, approved expansion session-local, rejection leaving candidate unchanged.
- [x] A runnable command-line driver (or test-fixture entry) demonstrates occupy → two single-voice writes → handoff against the fixture, printing the accumulated candidate diff and final proposal — proving the seam carries the issue-25 state model in-process.

## Work log — 2026-09-07

Completed and verified the protocol-neutral `PatternCapability` seam already
connected to the application service. The facade exposes exactly the five
Pattern-mode tools, binds one Pattern and immutable baseline, serializes sparse
semantic Score Context, accumulates complete single-channel segments in a
private candidate, enforces raw six-field preservation for unsupported tracker
content, and freezes a normalized immutable proposal on handoff.

The occupancy implementation now owns only the lease it acquired, rejects
reentrant calls, applies short call-level exclusion, retains and renews promoted
sessions, pauses expiry during range approval, and safely handles handoff,
abort, read-only release, timeout, stale dependencies, and force-release. Tests
also verify native Undo/Redo blocking during retained occupancy and atomic
whole-proposal Apply with one native Undo entry.

Run `./build-local.ps1 -Test` from the repository root. It builds the x64 Debug
application, runs the fixture-driven capability suite, prints `PASS`, then
prints the accumulated two-voice candidate diff and final proposal JSON.

Verification:

- `./build-local.ps1 -Test` — PASS.
- `python test-fixtures/validate_fixture.py` — all 131 checks passed, including playback.
- `python -m unittest discover -s sidecar -v` — 10 passed; the opt-in native integration test for the later end-to-end ticket was skipped.
- `libopenmpt_test.exe` was built and the full upstream suite was run once. It reaches the end but returns 255 because two pre-existing locale-transcode assertions (`tests_string_transcode.hpp` lines 138 and 248) fail under this host's Chinese Windows locale; all reported non-locale cases pass, and the failure does not touch the Pattern capability files.

Code review was intentionally not started, per the owner's requested pause
point.

## Code review — 2026-09-07

Two-axis review (standards, spec) of commit 579ffd7e5. Standards axis found no
documented-standard violations. Spec axis confirmed the five tools, occupancy,
signature, envelope and Apply behaviour present and tested; findings below.

Fixes applied:

- `ending_reminder` is now written exactly once per agent-facing result, at the
  `Call` boundary (and the resumed expansion result). `AI::Failure` no longer adds
  it, so transport/attachment failures and owner-side callbacks (Review/Reject/
  Apply) no longer carry agent session guidance.
- The volume column maximum (64, universal across supported module formats —
  ModSpecifications has no per-format field) is one named `MaxVolume` constant used
  by both Score Context and validation.
- `build-local.ps1` guards both report reads with clear errors when the test runner
  produced no report, and collapses the three env-var save/restore pairs into one
  loop.

Recorded design interpretations (review findings accepted as-is):

- Session-less writes are refused with `occupancyLost`: the write envelope, baseline
  and candidate are born at session binding, so a write cannot precede a session;
  issue 23's "short occupancy per call" is realized on the context read that starts
  every session.
- Expansion approval does not literally pause the in-flight call: on the single UI
  thread the call returns `pending_approval` and ends, the timeout is paused while
  the request is pending, and the exact request is replayed on approval.
- While a frozen proposal exists, fresh session-less reads return `busy`: one
  proposal at a time; Reject/Apply reopens reading, and the agent re-reads after
  Reject.
- The tool allowlist early in `Dispatch` gives unknown tools a clean `unsupported`
  error before any session state is touched; the terminal `Failure("unsupported")`
  after the chain is an unreachable guard (and keeps every path returning).
- `SameRaw` compares all six `ModCommand` fields unconditionally — deliberately
  stricter than upstream `ModCommand::operator==` (which ignores vol/param when
  commands are NONE) because the ticket requires byte-for-byte preservation even of
  empty-command parameters. `ModCommand` has exactly these six data members.
- The demo driver rides the test binary via the `OPENMPT_AI_DEMO_REPORT` env var, as
  ticket AC 5 explicitly allows ("or test-fixture entry"); the guarded script reads
  make an early test failure loud instead of confusing.

Verification after fixes: `./build-local.ps1 -Test` — PASS, demo diff and proposal
printed.
