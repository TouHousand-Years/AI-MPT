# 28: MPTM collaboration fixture

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** A purpose-built but musically credible MPTM test module for the vertical slice, per issue 20: fixed meter and tempo, existing piano, bass, and harmony-oriented sounds, and two resettable four-to-eight-bar starting states — (A) melody with empty harmony voices, and (B) harmony with an empty melody voice. Each target Pattern appears exactly once in the unchanged original Order list so no shared-Pattern consequence is disguised as per-Order-instance editing. The fixture lives with the project's test assets and is usable by both automated tests and the owner's manual verification.

**Blocked by:** None (can start immediately)

**Status:** resolved

## Acceptance criteria (demo to owner)

- [x] Opening the fixture in the app shows state A (melody present, harmony voices empty) and state B (harmony present, melody voice empty) with a documented, reliable way to reset to each.
- [x] The Order list contains each target Pattern exactly once; meter and tempo are fixed and stated in the fixture notes.
- [x] Both states play back as recognizable music using only pre-existing instrument/sample resources — no placeholder noise, no missing-sample warnings.
- [x] The module loads, plays, and round-trips through Save As / close / reopen with content intact (baseline check, not yet AI-touched).

## Work log — 2026-09-05

Delivered `test-fixtures/`:

- `ai-collaboration fixture` at `ai-collab-fixture.mptm`: 4/4 at 125 BPM
  (classic tempo mode, speed 6, 4 rows per beat), two 128-row (8-bar)
  patterns over a C Am F G progression — Pattern 0 "AI State A - Melody"
  (piano melody + bass, harmony channels empty) and Pattern 1
  "AI State B - Harmony" (two pad voices with note-offs + bass, melody
  channel empty); order list `[0, 1]`, each target Pattern exactly once.
  Embedded synthesized E-piano, warm pad, and round bass samples; song
  message documents timing, channels, both states, and the reset procedure.
- `generate_fixture.py`: deterministic generator that writes the MPTM
  byte-for-byte (IT/MPTM structures, STPM song extensions, Ssb sequence
  block), doubling as the documented reset mechanism.
- `validate_fixture.py`: 131 structural + playback checks (independent
  parser, openmpt123 render, tuning spot checks via Goertzel).
- `README.md`: states, channels, reset procedure, regeneration and manual
  verification checklist.

Verification: openmpt123 renders both states at the expected duration and
pitch (per-state spectral spot checks); the fixture was opened in the local
OpenMPT build (all document properties correct), saved via Save As, closed,
reopened, and the re-render of the copy matched the original render at
~-80 dB relative difference. The pristine file was restored after the
round-trip test.
