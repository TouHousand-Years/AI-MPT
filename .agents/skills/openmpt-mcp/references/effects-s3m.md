# S3M effect column and volume column

Self-contained reference for authoring the S3M effect column and volume column (`.s3m`) through the Pattern MCP. Everything needed is in this file; S3M uses a command set entirely different from MOD and XM, so do not carry letters over from another format's file.

## Before you write

1. Read `context.format.effect_commands` and `context.format.volume_commands` for the bound document. Each lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Use those IDs in `effect_command` / `volume_command`; the letters below are only the tracker display notation.
2. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.
3. The catalog is OpenMPT's full S3M set. Commands marked *non-ST3* below are not part of original Scream Tracker 3, but OpenMPT still exposes and plays them. The published catalog, not this list, is authoritative.

## How to read the notation

- An effect is one letter plus a parameter, written `Axy`. Uppercase letters address the effect column; lowercase letters address the volume column.
- `xx` is a two-digit hexadecimal value read as one byte; `xy` is two independent hexadecimal nibbles. Volume column parameters are decimal, matching the range the tool publishes.
- In the raw cell, `effect_parameter` holds the plain integer value of the digits: `D05` → 5, `D20` → 32, `H8F` → 143. `volume` holds the decimal volume-column value.
- A single command family shares one raw command. Write the catalog ID for the base letter and put the whole parameter in the field: `S9x` → the `s3mcmdex` ID with `effect_parameter` = `0x90 | x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Stay inside the documented ranges.

## Frequency units

Linear frequency slides are not available in S3M, so one unit of a pitch slide (`Exx`, `Fxx`, `Gxx`) is one *period*, a metric inverse to frequency: the lower the note, the smaller the audible change. A period is 3,546,895 ÷ frequency. Extra-fine units are 4× finer than fine units.

## Effect parameter tables

### Vibrato / tremolo / panbrello waveform (`S3x`, `S4x`, `S5x`)

| Parameter | Waveform |
| --- | --- |
| 0 (default) | Sine, retriggered on each new note |
| 1 | Sawtooth, retriggered |
| 2 | Square, retriggered |
| 3 | Random, retriggered — avoid |

Each waveform is 64 points long; the speed parameter advances by that many points per tick.

### Retrigger volume (`Qxy`, the `x` nibble)

| x | Effect | x | Effect |
| --- | --- | --- | --- |
| 0 | No volume change | 8 | No volume change |
| 1 | Volume − 1 | 9 | Volume + 1 |
| 2 | Volume − 2 | A | Volume + 2 |
| 3 | Volume − 4 | B | Volume + 4 |
| 4 | Volume − 8 | C | Volume + 8 |
| 5 | Volume − 16 | D | Volume + 16 |
| 6 | Volume × ⅔ | E | Volume × 1.5 |
| 7 | Volume × ½ | F | Volume × 2 |

### Sound control (`S9x`)

| Parameter | Name | Description |
| --- | --- | --- |
| 0 / 1 | Surround Off / On | Enables or disables surround playback on the channel. |
| 8 / 9 | Reverb Off / On | Per-channel reverb. Discouraged; prefer a reverb plugin. |
| A / B | Center / Quad Surround | Switches the global surround mode. |
| C / D | Global / Local Filters | Sets filter mode for all channels when a resonant filter is active. |
| E / F | Play Forward / Backward | Forces the current sample's playback direction. |

In S3M these are ModPlug extensions rather than original Scream Tracker 3 commands.

## Effect column commands

All parameters are hexadecimal. "Mem" is behaviour at parameter 0: **Yes** recalls the command's own last non-zero parameter, **—** means zero has its own literal meaning, and **Global** means the command recalls any previous non-zero parameter in the same column. An entry marked *non-ST3* is available in OpenMPT's S3M but is not an original Scream Tracker 3 command.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `Axx` | Set Speed | `speed` | No | Sets the module Speed (ticks per row). |
| `Bxx` | Position Jump | `positionjump` | — | Jumps to Order position `xx`; `B00` restarts. On the same row as `Cxx`, `Bxx` selects the Pattern `Cxx` breaks into. |
| `Cxx` | Pattern Break | `patternbreak` | — | Jumps to row `xx` of the next Pattern. Range `00`–`3F`; higher values are ignored. |
| `Dxy` | Volume Slide / Fine Volume Slide | `volumeslide` | Global | `D0y` down by `y`, `Dx0` up by `x`, every tick except the first; `DFy`/`DxF` apply only on the first tick. Volume caps at 64. |
| `Exx` | Portamento Down / Fine / Extra Fine | `portamentodown` | Global | `Exx` slides down every tick except the first; `EFx` on the first tick only; `EEx` at 4× the precision of `EFx`. |
| `Fxx` | Portamento Up / Fine / Extra Fine | `portamentoup` | Global | `Fxx` slides up every tick except the first; `FFx` on the first tick only; `FEx` at 4× the precision of `FFx`. |
| `Gxx` | Tone Portamento | `toneportamento` | Yes | Slides the previous note toward the current note by `xx` per tick except the first. |
| `Hxy` | Vibrato | `vibrato` | Yes | Vibrato, speed `x`, depth `y`, waveform from `S3x`. Shares memory with `Uxy`. |
| `Ixy` | Tremor | `tremor` | Global | Volume on for `x` ticks, off for `y` ticks. |
| `Jxy` | Arpeggio | `arpeggio` | Global | Cycles within one row between the current note, +`x` semitones, +`y` semitones. |
| `Kxy` | Volume Slide + Vibrato | `vibratovol` | Global | `Dxy` volume slide plus `H00`. |
| `Lxy` | Volume Slide + Tone Portamento | `toneportavol` | Global | `Dxy` volume slide plus `G00`. |
| `Mxx` | Set Channel Volume | `channelvolume` | — | *non-ST3* Channel volume multiplier, `00` off to `40` full. |
| `Nxy` | Channel Volume Slide | `channelvolslide` | Yes | *non-ST3* Like `Dxy`, applied to channel volume. |
| `Oxx` | Sample Offset | `offset` | Yes | Starts the sample at `xx × 256`. Requires a note in the same cell. |
| `Pxy` | Panning Slide / Fine Panning Slide | `panningslide` | Yes | *non-ST3* `P0y` slides right by `y`, `Px0` slides left by `x`, every tick except the first; `PFy`/`PxF` apply on the first tick only. |
| `Qxy` | Retrigger | `retrig` | Global | Retriggers every `y` ticks and changes volume by the `x` nibble (see the retrigger volume table). |
| `Rxy` | Tremolo | `tremolo` | Global | Volume tremolo, speed `x`, depth `y`, waveform from `S4x`. |
| `S1x` | Glissando Control | `s3mcmdex` | — | `S10` off, `S11` on. Quirky and not widely supported. |
| `S2x` | Set Finetune | `s3mcmdex` | — | Legacy command; overrides the current sample's C-5 frequency with a MOD finetune value. |
| `S3x` | Set Vibrato Waveform | `s3mcmdex` | — | Selects the waveform table above for later `Hxy`. |
| `S4x` | Set Tremolo Waveform | `s3mcmdex` | — | Selects the waveform table above for later `Rxy`. |
| `S5x` | Set Panbrello Waveform | `s3mcmdex` | — | *non-ST3* Selects the waveform table above for later `Yxy`. |
| `S6x` | Fine Pattern Delay | `s3mcmdex` | — | Extends the row by `x` ticks; multiple `S6x` on a row sum. |
| `S8x` | Set Panning | `s3mcmdex` | — | Coarse panning, `0` left to `F` right. `Xxx` is finer. |
| `S9x` | Sound Control | `s3mcmdex` | — | *non-ST3* Runs a sound control command (see the table above). |
| `SAx` | High Offset | `s3mcmdex` | — | *non-ST3* Adds `x × 65536` to all following `Oxx` offsets. |
| `SB0` | Pattern Loop Start | `s3mcmdex` | — | Marks the `SBx` loop start. |
| `SBx` | Pattern Loop | `s3mcmdex` | — | Jumps back to the `SB0` row until `x` jumps total. Cannot span Patterns. Range `1`–`F`. |
| `SCx` | Note Cut | `s3mcmdex` | — | Stops the sample after `x` ticks; ignored if `x` is 0 or ≥ Speed. |
| `SDx` | Note Delay | `s3mcmdex` | — | Delays the cell's note/instrument by `x` ticks; ignored if `x` is 0 or ≥ Speed. |
| `SEx` | Pattern Delay | `s3mcmdex` | — | Repeats the row `x` times without retriggering notes; only the leftmost `SEx` counts. |
| `T0x` | Decrease Tempo | `tempo` | Yes | Lowers Tempo by `x` BPM on every tick except the first. |
| `T1x` | Increase Tempo | `tempo` | Yes | Raises Tempo by `x` BPM on every tick except the first. |
| `Txx` | Set Tempo | `tempo` | No | Sets Tempo when `xx` ≥ `20`. |
| `Uxy` | Fine Vibrato | `finevibrato` | Yes | Like `Hxy` with 4× precision. Shares memory with `Hxy`. |
| `Vxx` | Set Global Volume | `globalvolume` | — | Song global volume, `00` off to `40` full. |
| `Wxy` | Global Volume Slide | `globalvolslide` | Yes | *non-ST3* Like `Dxy`, applied to the global volume. |
| `Xxx` | Set Panning | `panning8` | — | Channel panning, `00` left to `80` right. `XA4` (a non-ST3 extension) enables surround on the channel; any other `Xxx` on that channel disables it. |
| `Yxy` | Panbrello | `panbrello` | Yes | *non-ST3* Panning oscillates with speed `x`, depth `y`, waveform from `S5x`. |
| `Zxx` | MIDI Macro | `midi` | — | *non-ST3* Runs a MIDI macro. S3M files do not store macros, so only the default macro configuration applies, and the Pattern tools cannot read or change it — the audible result is not verifiable here. |

The base and extended catalogs both expose a `tempo` command ID that carries `T0x`, `T1x` and `Txx`; the sub-command is the high nibble of `effect_parameter`. The base catalog also contains an internal `dummy` entry that is never useful to write.

## Volume column commands

All parameters are decimal, matching the published range. S3M's volume column is intentionally small.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `vxx` | Set Volume | `volume` | No | Sets note volume, 0 off to 64 full. |
| `pxx` | Set Panning | `panning` | No | Channel panning, 0 left to 64 right. *non-ST3* |

Volume slides, fine slides, vibrato depth, tone portamento and panning slides that other formats allow in the volume column are **not** available in S3M. Express them in the effect column (`Dxy`, `Gxx`, `Hxy`, `Pxy`).

## Authoring notes

- **Whole families share one ID.** `S1x`–`SEx` all use `s3mcmdex`; `T0x`, `T1x` and `Txx` all use `tempo`. The sub-command is the high nibble of `effect_parameter`.
- **S3M uses global effect memory.** Commands marked **Global** above recall any previous non-zero parameter written in the same column, not just their own. This differs from MOD/XM/IT and is the most common source of unintended slides.
- **Global commands change playback flow, not just the cell.** `Bxx`, `Cxx`, `SBx`, `SEx`, `Axx`, `Txx`, `Vxx`, `Wxy` and `S6x` affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **AdLib / OPL3 instruments** are supported in S3M, but `Oxx`, `S9x`, `SAx` and `XA4` have no effect on them, `Mxx` and `Nxy` work only with them in OpenMPT, and `Xxx`/`pxx` collapse to hard left, center and hard right.
- The Pattern tools cannot verify audio. A written command is only as correct as the catalog entry and the range documented above.
