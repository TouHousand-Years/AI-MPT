# 34: Vertical-slice verification: Save As, reopen, human keep-verdict

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The closing acceptance pass for the spec (issue 26) and the map destination: a complete owner-driven verification session over the issue-20 required-verification checklist, ending with manual Save As, close, and reopen of the test copy, and the owner's verdict on whether at least one generated result is worth keeping. Any defects found are fixed under this ticket; no new capability is added.

**Blocked by:** 33 (End-to-end Agent loop: melody↔harmony both directions)

**Status:** ready-for-agent

## Acceptance criteria (owner verification session, per issue 20)

- [ ] Both melody-to-harmony and harmony-to-melody workflows pass through the same MCP and proposal loop.
- [ ] Explicit-selection and no-selection whole-Pattern scopes, single-voice calls, multi-call candidate accumulation, and approved range expansion all verified.
- [ ] Short and retained occupancy, permitted navigation/playback, blocked human writes, explicit handoff/abort, timeout, and high-priority human release all verified.
- [ ] Unchanged candidate after one failed call and complete invalidation after occupancy or revision loss verified.
- [ ] Visible failure without document mutation for a stale proposal and for unsupported tracker-only content verified.
- [ ] One atomic Apply, one revision increment, one ordinary Undo step, and no document change after Reject or failed Apply verified.
- [ ] Post-Apply playback, human musical feedback followed by a fresh proposal, and successful Save As + close/reopen with correct MPTM content and playback verified.
- [ ] The owner records the verdict that at least one generated result is worth retaining for further work.
- [ ] Issue 26 and the map destination are marked complete only after every item above passes.
