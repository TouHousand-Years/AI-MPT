# AI collaboration MPTM fixture

`ai-collab-fixture.mptm` is the purpose-built test module for the first
vertical slice ([issue 20](../.scratch/ai-collaborative-openmpt/issues/20-define-first-personally-useful-vertical-slice.md),
[ticket 28](../.scratch/ai-collaborative-openmpt/issues/28-create-mptm-collaboration-fixture.md)):
a musically credible song that gives the melody-to-harmony and
harmony-to-melody collaboration workflows a repeatable starting point.

## Fixed timing

| Property | Value |
| --- | --- |
| Meter | 4/4 — 4 rows per beat, 16 rows per bar |
| Tempo | 125 BPM, classic tempo mode, speed 6 |
| Patterns | 128 rows = 8 bars each |
| Key / progression | C major: C Am F G, twice |

There are no tempo, speed, or jump effects anywhere in the module, and each
target Pattern appears exactly once in the order list (`0, 1`), so an AI edit
to a Pattern can never hide behind a shared-Pattern second occurrence.

## Instruments and channels

| Channel | Name | Instrument | Sample |
| --- | --- | --- | --- |
| 1 | Melody | 1 "E-Piano" | 1 |
| 2 | Harmony 1 | 2 "Warm Pad" | 2 |
| 3 | Harmony 2 | 2 "Warm Pad" | 2 |
| 4 | Bass | 3 "Round Bass" | 3 |

All sounds are embedded 16-bit samples synthesized by the generator; there are
no external samples, no missing-sample warnings, and no placeholder noise.

## Starting states

Each state is one Pattern; the other pattern always remains pristine in the
file.

- **Pattern 0 "AI State A - Melody"** — melody (channel 1) and bass (channel 4)
  written; harmony voices (channels 2 and 3) are empty. Use for the
  *melody → harmony* test.
- **Pattern 1 "AI State B - Harmony"** — two-part harmony (channels 2 and 3,
  whole-bar pad chords with note-offs) and bass (channel 4) written; the
  melody voice (channel 1) is empty. Use for the *harmony → melody* test.

Both states are also documented in the module's song message (visible in
OpenMPT's song message / comments view).

## Resetting a state

The pristine file is the reset. The same information is in the song message.

1. Work on an in-memory copy: open the fixture, let the AI collaborate, then
   use **Save As** for any copy worth keeping. Never save over
   `ai-collab-fixture.mptm`.
2. To reset, close the module **without saving** (or undo by hand) and reopen
   the pristine file — both patterns are back in their original states.
3. If a working copy on disk needs resetting, delete it and re-copy the
   pristine file from the repository (or regenerate it, see below).

## Regenerating and validating

The generator is deterministic; running it reproduces the file
byte-for-byte.

```powershell
python test-fixtures/generate_fixture.py          # regenerate ai-collab-fixture.mptm
python test-fixtures/validate_fixture.py          # structural + playback checks
```

The validator parses the module with its own parser (separate from the
generator's writer code) and checks format identity, order list, per-channel
pattern contents, instruments/samples, fixed meter/tempo, and determinism.
Note the expected values (names, tempo, notes) are shared with the generator
by design — the checks that anchor independently of the generator are the
rendered-audio ones: when `openmpt123.exe` from the local build is available,
the validator renders the module and checks duration, audibility, and the
tuning of both states.

## Manual verification checklist (owner)

1. Open `ai-collab-fixture.mptm` in the local OpenMPT build — no warnings,
   4 channels, 2 patterns, 3 instruments, song message readable.
2. Play pattern 0: recognizable melody + bass, harmony channels empty.
3. Play pattern 1: pad harmony + bass, melody channel empty.
4. Save As a working copy, close, reopen: content and playback intact.
