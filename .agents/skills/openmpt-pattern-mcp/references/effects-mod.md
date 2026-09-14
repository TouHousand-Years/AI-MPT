# MOD effect column

Self-contained reference for authoring the MOD effect column (`.mod`) through the Pattern MCP. MOD has **no volume column**, so every volume-related move is made in the effect column. Letter notation is reassigned between formats; do not carry it over from another format's file.

## Before you write

1. Read `context.format.effect_commands` for the bound document. It lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Use those IDs in `effect_command`; the letters below are only the tracker display notation.
2. MOD publishes **no** volume commands (`context.format.volume_commands` contains only `none`). A nonzero `volume_command` is rejected. Keep `volume_command: 0, volume: 0`.
3. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.

## How to read the notation

- An effect is one letter plus a parameter, written `Axy`. Uppercase letters address the effect column.
- `xx` is a two-digit hexadecimal value read as one byte. `xy` is two independent hexadecimal nibbles (`x` = high nibble, `y` = low nibble). `x` alone is a single hexadecimal digit.
- In the raw cell, `effect_parameter` holds the plain integer value of those digits: `A05` → `effect_parameter: 5`; `A20` → `effect_parameter: 32`; `F7D` → `effect_parameter: 125`.
- A single `E` command family shares one raw command. Write `effect_command` = the catalog's `modcmdex` entry and put the whole two-digit parameter in `effect_parameter`: `E12` → `effect_parameter: 18` (0x12 = 0x10 sub-command + 0x2 value).
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Values outside the documented range are not recommended, because other players may read them differently.

## Frequency units

With the default MOD mixing, one unit of a pitch slide (`1xx`, `2xx`, `3xx`) is one *period*, a metric inverse to frequency: the lower the note, the smaller the audible change. Linear frequency slides are not available in MOD.

## Effect parameter tables

### Vibrato / tremolo waveform (`E4x`, `E7x`)

| Parameter | Waveform |
| --- | --- |
| 0 (default) | Sine, retriggered on each new note |
| 1 | Sawtooth, retriggered |
| 2 | Square, retriggered |
| 3 | Random, retriggered — not supported by ProTracker; avoid |
| 4 | Sine, continue from last position |
| 5 | Sawtooth, continue |
| 6 | Square, continue |
| 7 | Random, continue — not supported by ProTracker; avoid |

Each waveform is 64 points long; the speed parameter advances by that many points per tick, so speed 2 repeats after 32 ticks.

## Effect column commands

All parameters are hexadecimal. "Mem" is effect memory at parameter 0: **Yes** recalls the command's own last non-zero parameter, **No** does nothing, **—** means zero has its own literal meaning.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `0xy` | Arpeggio | `arpeggio` | No | Cycles within one row between the current note, current note + `x` semitones, and current note + `y` semitones. |
| `1xx` | Portamento Up | `portamentoup` | No | Raises pitch by `xx` units on every tick except the first. |
| `2xx` | Portamento Down | `portamentodown` | No | Lowers pitch by `xx` units on every tick except the first. |
| `3xx` | Tone Portamento | `toneportamento` | Yes | Slides the previous note's pitch toward the current note by `xx` units on every tick except the first. |
| `4xy` | Vibrato | `vibrato` | Yes | Vibrato with speed `x`, depth `y`, using the waveform set by `E4x`. |
| `5xy` | Volume Slide + Tone Portamento | `toneportavol` | No | `Axy` volume slide combined with a `300` tone portamento. |
| `6xy` | Volume Slide + Vibrato | `vibratovol` | No | `Axy` volume slide combined with a `400` vibrato. |
| `7xy` | Tremolo | `tremolo` | Yes | Volume tremolo with speed `x`, depth `y`, using the waveform set by `E7x`. |
| `8xx` | Set Panning | `panning8` | — | Channel panning, `00` hard left to `FF` hard right. |
| `9xx` | Sample Offset | `offset` | Yes | Starts the sample at `xx × 256` samples. Has no effect unless the same cell also contains a note. |
| `Axy` | Volume Slide | `volumeslide` | No | `A0y` lowers note volume by `y` per tick; `Ax0` raises it by `x` per tick. Both on every tick except the first. |
| `Bxx` | Position Jump | `positionjump` | — | Jumps to Order position `xx`; `B00` restarts from the first Order. Range `00`–`7F`. On the same row as `Dxx`, `Bxx` selects the Pattern that `Dxx` breaks into. |
| `Cxx` | Set Volume | `volume` | — | Sets the current note volume, `00` off to `40` full. |
| `Dxx` | Pattern Break | `patternbreak` | — | Jumps to row `xx` of the next Order's Pattern. Range `00`–`3F`. |
| `E0x` | Set Filter | `modcmdex` | — | Amiga LED lowpass filter: `E00` enables, `E01` disables. Only audible when the Amiga resampler is active. Leave off unless explicitly required. |
| `E1x` | Fine Portamento Up | `modcmdex` | No | Like `1xx`, applied only on the first tick of the row. |
| `E2x` | Fine Portamento Down | `modcmdex` | No | Like `2xx`, applied only on the first tick of the row. |
| `E3x` | Glissando Control | `modcmdex` | — | `E30` disables, `E31` enables semitone-quantized tone portamento. Quirky and not widely supported. |
| `E4x` | Set Vibrato Waveform | `modcmdex` | — | Selects the waveform table above for later `4xy` commands. |
| `E5x` | Set Finetune | `modcmdex` | — | Temporarily overrides the playing note's finetune; only works when the same cell holds a note. |
| `E60` | Pattern Loop Start | `modcmdex` | — | Marks the row used as the loop start for `E6x`. |
| `E6x` | Pattern Loop | `modcmdex` | — | Jumps back to the `E60` row until `x` total jumps have happened. Loops cannot span Patterns. Range `1`–`F`. |
| `E7x` | Set Tremolo Waveform | `modcmdex` | — | Selects the waveform table above for later `7xy` commands. |
| `E8x` | Set Panning | `modcmdex` | — | Coarse channel panning, `0` left to `F` right. |
| `E9x` | Retrigger | `modcmdex` | No | Retriggers the note every `x` ticks. |
| `EAx` | Fine Volume Slide Up | `modcmdex` | No | Like `Ax0`, applied only on the first tick of the row. |
| `EBx` | Fine Volume Slide Down | `modcmdex` | No | Like `A0y`, applied only on the first tick of the row. |
| `ECx` | Note Cut | `modcmdex` | — | Sets note volume to 0 after `x` ticks; ignored if `x` is at least the current Speed. |
| `EDx` | Note Delay | `modcmdex` | — | Delays the cell's note or instrument change by `x` ticks; if `x` is at least the current Speed, nothing in the cell plays. |
| `EEx` | Pattern Delay | `modcmdex` | — | Repeats the current row `x` times without retriggering notes; effects still run. Only the rightmost `EEx` on a row counts. |
| `EFx` | Invert Loop | `modcmdex` | — | **Destructive.** Walks the sample loop and inverts sample points at speed `x`; the change is applied to the sample data at playback and cannot be undone automatically. `EF0` cancels it. Avoid. |
| `Fxx` | Set Speed / Tempo | `speed` (id for `xx` < `20`), `tempo` (id for `xx` ≥ `20`) | — | `xx` < `20` sets Speed (ticks per row); `xx` ≥ `20` sets Tempo. The catalog lists `speed` and `tempo` as two separate IDs that both display as `F`, so pick the ID that matches the value. Avoid `20` and `00`. |

## Authoring notes

- **Two IDs share the letter `F`.** `context.format.effect_commands` contains separate `speed` and `tempo` entries. For a value below 32 use the `speed` ID; for 32 and above use the `tempo` ID.
- **The `E` family is one command ID.** `E0x` through `EFx` all use the single `modcmdex` ID; the sub-command lives in the high nibble of `effect_parameter`.
- **Global commands change playback flow, not just the cell.** `Bxx`, `Dxx`, `E6x`, `EEx` and `Fxx` affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **Volume must go through the effect column** (`Cxx`, `Axy`, `EAx`, `EBx`) because MOD has no volume column.
- **Effects with effect memory** (`3xx`, `4xy`, `7xy`, `9xx`) continue a previous slide or oscillation when written with parameter `00`/`000`.
- The Pattern tools cannot verify audio. A written effect is only as correct as the catalog entry and the range documented above.
