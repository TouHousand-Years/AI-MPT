# 33: End-to-end Agent loop: melody↔harmony both directions

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The complete live loop from issue 20 wired together: a real Agent client connects through the MCP Sidecar (31) to the running app (30), works the issue-28 fixture through the capability seam (29) while the owner edits and reviews in the synchronized Tracker/Piano Roll editor (27, 32). Both directions are exercised through the same proposal workflow: the owner edits/selects a main melody and asks the Agent for harmony; then selects harmony and asks for a main melody. Multi-voice results accumulate over several single-voice calls under retained cross-call occupancy; handoff, human whole-proposal review, Apply, playback, Undo, and natural-language feedback retries (fresh revision-bound proposals) all work end to end for the first time on real transport.

**Blocked by:** 27 (Piano Roll pane into the main development line), 28 (MPTM collaboration fixture), 31 (MCP Sidecar process), 32 (Proposal review UI and atomic Apply)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner, live session)

- [ ] Melody-to-harmony: owner selects the melody in fixture state A; the Agent reads context, writes harmony voices over multiple single-voice calls under one retained occupancy; handoff produces one proposal; owner Applies, hears it through normal playback, and can reverse it with one Undo.
- [ ] Harmony-to-melody: the same loop in fixture state B producing a melody over the owner's harmony.
- [ ] During retained occupancy the owner can navigate, inspect in both projections, and play the committed Pattern (never the private candidate), while project writes, Undo, and Redo are blocked; bound Pattern, base revision, and initial write range do not follow owner navigation.
- [ ] Owner force-release mid-session and occupancy timeout are each demonstrated once: the candidate is fully invalidated, no document mutation occurs, and subsequent Agent calls fail with occupancy loss.
- [ ] A failed mutation call leaves the candidate unchanged (Agent retries corrected); a rejected proposal followed by natural-language feedback produces a fresh proposal from the then-current revision.
- [ ] Range expansion beyond an explicit selection triggers the non-modal approval surface; approval enlarges the session-local envelope, rejection leaves the candidate unchanged.
