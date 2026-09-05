# 28: MPTM collaboration fixture

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** A purpose-built but musically credible MPTM test module for the vertical slice, per issue 20: fixed meter and tempo, existing piano, bass, and harmony-oriented sounds, and two resettable four-to-eight-bar starting states — (A) melody with empty harmony voices, and (B) harmony with an empty melody voice. Each target Pattern appears exactly once in the unchanged original Order list so no shared-Pattern consequence is disguised as per-Order-instance editing. The fixture lives with the project's test assets and is usable by both automated tests and the owner's manual verification.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [ ] Opening the fixture in the app shows state A (melody present, harmony voices empty) and state B (harmony present, melody voice empty) with a documented, reliable way to reset to each.
- [ ] The Order list contains each target Pattern exactly once; meter and tempo are fixed and stated in the fixture notes.
- [ ] Both states play back as recognizable music using only pre-existing instrument/sample resources in the module — no placeholder noise, no missing-sample warnings.
- [ ] The module loads, plays, and round-trips through Save As / close / reopen with content intact (baseline check, not yet AI-touched).
