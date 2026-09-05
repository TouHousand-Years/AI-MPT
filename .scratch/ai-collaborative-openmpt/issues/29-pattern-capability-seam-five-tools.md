# 29: Pattern capability seam with the five Pattern-mode tools

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The protocol-neutral Application Capability facade above `CModDoc` / `CSoundFile` carrying exactly the five Pattern-mode tools from issue 23, callable in-process: `get_pattern_context` (immutable sparse semantic Score Context of the bound Pattern — identity, dimensions, timing context, module-format limits, read-only instrument/sample summaries, every non-empty cell with stable semantic names plus raw values), `replace_pattern_segment` (one contiguous row segment of one channel per call into a private candidate; complete desired cells; all-or-nothing validation; unsupported tracker semantics preserved byte-for-byte or the whole call fails), `handoff_for_review` (freeze immutable proposal + release occupancy atomically), `abort_session`, and `release_occupancy`. Includes session binding (Pattern, baseline, initial write envelope from explicit `PatternRect` or whole Pattern), short per-call occupancy with `occupy=true` promotion to retained cross-call occupancy, the five-minute renewable timeout paused during expansion-approval waits, human force-release, the internal Pattern dependency signature with stale detection, and write-envelope expansion with the ask/always-approve preference hook. Blocked human writes/Undo/Redo during retained occupancy; reads never restricted by the write envelope but never cross the bound Pattern.

**Blocked by:** 28 (MPTM collaboration fixture)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [ ] Test suite (following the existing `mpt_tests_*` pattern, run against the issue-28 fixture) is green covering: Score Context contents and narrowing, single-channel contiguous segment writes, multi-call accumulation into one candidate and one normalized final diff, all-or-nothing failure with candidate exactly unchanged and typed row/field/value/reason feedback, and byte-for-byte preservation of unsupported content (including whole-call failure when a requested note would clobber a special event).
- [ ] Occupancy behavior verified by test: short occupancy per call, promotion via `occupy=true`, retention across calls, omission not releasing promotion, explicit handoff/abort/release endings, `release_occupancy` failing while candidate edits exist, timeout expiry, and human force-release invalidating token and candidate.
- [ ] Dependency-signature behavior verified by test: changes to the bound Pattern or its dependencies invalidate session/proposal (stale, never rebased); changes to an unrelated Pattern do not.
- [ ] Write-envelope behavior verified by test: explicit selection and whole-Pattern scopes, envelope expansion gated on approval, approved expansion session-local, rejection leaving candidate unchanged.
- [ ] A runnable command-line driver (or test-fixture entry) demonstrates occupy → two single-voice writes → handoff against the fixture, printing the accumulated candidate diff and final proposal — proving the seam carries the issue-25 state model in-process.
