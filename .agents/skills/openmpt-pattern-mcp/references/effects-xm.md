# XM effect column and volume column

Self-contained reference for authoring the XM effect column and volume column (`.xm`) through the Pattern MCP. Everything needed is in this file; letters are reassigned between formats, so do not consult another format's file.

## Before you write

1. Read `context.format.effect_commands` and `context.format.volume_commands` for the bound document. Each lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Use those IDs in `effect_command` / `volume_command`; the letters below are only the tracker display notation.
2. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.
3. The catalog is OpenMPT's full XM set. Commands marked *hack* below are not implemented by Fasttracker II, but OpenMPT still exposes and plays them. The published catalog, not this list, is authoritative.

## How to read the notation

- An effect is one letter plus a parameter, written `Axy`. Uppercase letters address the effect column; lowercase letters address the volume column.
- `xx` is a two-digit hexadecimal value read as one byte; `xy` is two independent hexadecimal nibbles. Volume column parameters are decimal, matching the range the tool publishes.
- In the raw cell, `effect_parameter` holds the plain integer value of the digits: `A05` → 5, `A20` → 32, `R0F` → 15. `volume` holds the decimal volume-column value.
- A single command family shares one raw command. Write the catalog ID for the base letter and put the whole parameter in the field: `X9x` → the `xfineportaupdown` ID with `effect_parameter` = `0x90 | x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Stay inside the documented ranges.

## Frequency units

XM uses linear frequency slides by default, so one unit of `1xx`, `2xx`, `3xx` is 1/16 semitone and one unit of `E1x`, `E2x` is 1/64 semitone. If linear slides are disabled, a unit is one *period*, computed as 3,579,364 ÷ frequency, and lower notes change less than higher notes.

## Effect parameter tables

### Vibrato / tremolo / panbrello waveform (`E4x`, `E7x`, `X5x`)

| Parameter | Waveform | Parameter | Waveform |
| --- | --- | --- | --- |
| 0 (default) | Sine, retriggered | 4 | Sine, continue |
| 1 | Sawtooth, retriggered | 5 | Sawtooth, continue |
| 2 | Square, retriggered | 6 | Square, continue |
| 3 | Random, retriggered — avoid | 7 | Random, continue — avoid |

"Retrigger" restarts the waveform on each new note; "continue" resumes from the last position and is exclusive to MOD and XM. Each waveform is 64 points long; speed is how many points per tick are advanced.

### Retrigger volume (`Rxy`, the `x` nibble)

| x | Effect | x | Effect |
| --- | --- | --- | --- |
| 0 | Recall the last `x` value | 8 | No volume change |
| 1 | Volume − 1 | 9 | Volume + 1 |
| 2 | Volume − 2 | A | Volume + 2 |
| 3 | Volume − 4 | B | Volume + 4 |
| 4 | Volume − 8 | C | Volume + 8 |
| 5 | Volume − 16 | D | Volume + 16 |
| 6 | Volume × ⅔ | E | Volume × 1.5 |
| 7 | Volume × ½ | F | Volume × 2 |

### Sound control (`X9x`)

| Parameter | Name | Description |
| --- | --- | --- |
| 0 / 1 | Surround Off / On | Enables or disables surround playback on the channel. |
| 8 / 9 | Reverb Off / On | Per-channel reverb. Discouraged; prefer a reverb plugin. |
| A / B | Center / Quad Surround | Switches the global surround mode. |
| C / D | Global / Local Filters | When a resonant filter is active, global filters persist until explicitly reset; local filters revert on the next note. |
| E / F | Play Forward / Backward | Forces the current sample's playback direction. |

## Effect column commands

All parameters are hexadecimal. "Mem" is effect memory at parameter 0: **Yes** recalls the command's own last non-zero parameter, **No** does nothing, **—** means zero has its own literal meaning. Entries marked *hack* are ModPlug extensions that Fasttracker II does not implement.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `0xy` | Arpeggio | `arpeggio` | No | Cycles within one row between the current note, +`x` semitones, +`y` semitones. |
| `1xx` | Portamento Up | `portamentoup` | Yes | Raises pitch by `xx` on every tick except the first. |
| `2xx` | Portamento Down | `portamentodown` | Yes | Lowers pitch by `xx` on every tick except the first. |
| `3xx` | Tone Portamento | `toneportamento` | Yes | Slides the previous note toward the current note by `xx` per tick except the first. |
| `4xy` | Vibrato | `vibrato` | Yes | Vibrato, speed `x`, depth `y`, waveform from `E4x`. |
| `5xy` | Volume Slide + Tone Portamento | `toneportavol` | Yes | `Axy` volume slide plus `300`. |
| `6xy` | Volume Slide + Vibrato | `vibratovol` | Yes | `Axy` volume slide plus `400`. |
| `7xy` | Tremolo | `tremolo` | Yes | Volume tremolo, speed `x`, depth `y`, waveform from `E7x`. |
| `8xx` | Set Panning | `panning8` | — | Sample panning, `00` left to `FF` right. A later instrument-column entry resets it to the sample default. |
| `9xx` | Sample Offset | `offset` | Yes | Starts the sample at `xx × 256`. Requires a note in the same cell. |
| `Axy` | Volume Slide | `volumeslide` | Yes | `A0y` down by `y`, `Ax0` up by `x`, every tick except the first. |
| `Bxx` | Position Jump | `positionjump` | — | Jumps to Order position `xx`. When `Dxx` is to its right on the same row, `Bxx` selects the Pattern `Dxx` breaks into. |
| `Cxx` | Set Volume | `volume` | — | Note volume, `00` off to `40` full. |
| `Dxx` | Pattern Break | `patternbreak` | — | Jumps to row `xx` of the next Pattern. Keep `xx` ≤ `3F` for Fasttracker II compatibility. |
| `E1x` | Fine Portamento Up | `modcmdex` | Yes | Like `1xx`, first tick only. |
| `E2x` | Fine Portamento Down | `modcmdex` | Yes | Like `2xx`, first tick only. |
| `E3x` | Glissando Control | `modcmdex` | — | `E30` off, `E31` on. Quirky and not widely supported. |
| `E4x` | Set Vibrato Waveform | `modcmdex` | — | Selects the waveform table above for later `4xy`. |
| `E5x` | Set Finetune | `modcmdex` | — | Temporary finetune override; only works with a note in the same cell. |
| `E60` | Pattern Loop Start | `modcmdex` | — | Marks the `E6x` loop start. A Fasttracker II bug also makes the following Pattern start from that row unless a `D00` closes the current Pattern. |
| `E6x` | Pattern Loop | `modcmdex` | — | Jumps back to the `E60` row until `x` jumps total. Cannot span Patterns. Range `1`–`F`. |
| `E7x` | Set Tremolo Waveform | `modcmdex` | — | Selects the waveform table above for later `7xy`. |
| `E8x` | Set Panning | `modcmdex` | — | Coarse panning, `0` left to `F` right. |
| `E9x` | Retrigger | `modcmdex` | No | Retriggers the note every `x` ticks. |
| `EAx` | Fine Volume Slide Up | `modcmdex` | Yes | Like `Ax0`, first tick only. |
| `EBx` | Fine Volume Slide Down | `modcmdex` | Yes | Like `A0y`, first tick only. |
| `ECx` | Note Cut | `modcmdex` | — | Volume 0 after `x` ticks; ignored if `x` ≥ Speed. |
| `EDx` | Note Delay | `modcmdex` | — | Delays the cell's note/instrument by `x` ticks. Buggy in Fasttracker II (portamento beside it is ignored); do not rely on the emulation. |
| `EEx` | Pattern Delay | `modcmdex` | — | Repeats the row `x` times without retriggering notes; only the rightmost `EEx` counts. |
| `EFx` | Set Active Macro | `modcmdex` | — | *hack* Selects the channel's active parametered macro. |
| `Fxx` | Set Speed / Tempo | `speed` (id for `xx` < `20`), `tempo` (id for `xx` ≥ `20`) | — | `xx` < `20` sets Speed; `xx` ≥ `20` sets Tempo. The catalog lists `speed` and `tempo` as two IDs that both display as `F`; pick the ID matching the value. Avoid `00`. |
| `Gxx` | Set Global Volume | `globalvolume` | — | Song global volume, `00` off to `40` full. |
| `Hxy` | Global Volume Slide | `globalvolslide` | Yes | Like `Axy`, applied to the global volume. |
| `Kxx` | Key Off | `keyoff` | — | Triggers Note Off after `xx` ticks. Avoid `00`, which interferes with other entries in the cell. |
| `Lxx` | Set Envelope Position | `setenvposition` | — | Sets the volume envelope position to `xx` ticks; also moves the panning envelope when its sustain point is enabled. |
| `Pxy` | Panning Slide | `panningslide` | Yes | `P0y` slides left by `y`, `Px0` slides right by `x`, first tick only. |
| `Rxy` | Retrigger | `retrig` | Yes | Retriggers every `y` ticks and changes volume by the `x` nibble (see the retrigger volume table). Buggy with a volume command in the same cell. |
| `Txy` | Tremor | `tremor` | Yes | Volume on for `x`+1 ticks, off for `y`+1, every tick except the first. |
| `X1x` | Extra Fine Portamento Up | `xfineportaupdown` | Yes | Like `E1x` with 4× precision. |
| `X2x` | Extra Fine Portamento Down | `xfineportaupdown` | Yes | Like `E2x` with 4× precision. |
| `X5x` | Set Panbrello Waveform | `xfineportaupdown` | — | *hack* Selects the waveform table above for later `Yxy`. |
| `X6x` | Fine Pattern Delay | `xfineportaupdown` | — | *hack* Extends the row by `x` ticks; multiple `X6x` on a row sum. |
| `X9x` | Sound Control | `xfineportaupdown` | — | *hack* Runs a sound control command (see the table above). |
| `XAx` | High Offset | `xfineportaupdown` | — | *hack* Adds `x × 65536` to all following `9xx` offsets. |
| `Yxy` | Panbrello | `panbrello` | Yes | *hack* Panning oscillates with speed `x`, depth `y`, waveform from `X5x`. |
| `Zxx` | MIDI Macro | `midi` | — | *hack* Runs a MIDI macro. The macro itself lives in module configuration the Pattern tools cannot read or change, so the audible result is not verifiable here. |
| `\xx` | Smooth MIDI Macro | `smoothmidi` | — | *hack* As `Zxx`, interpolated over the row. |
| `#xx` | Parameter Extension | `xparam` | — | *hack* Extends the parameter of a preceding position jump, pattern break, sample offset or tempo command by combining bytes. |

The catalog may also list an internal `dummy` command; it is an import placeholder and is never useful to write.

## Volume column commands

All parameters are decimal, and the tool publishes the exact range. In XM the maximum is 64 for `vxx`, 15 for `pxx`, and 15 for every other volume command.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `vxx` | Set Volume | `volume` | No | Sets note volume, 0 off to 64 full. The plain volume command; the remaining commands below are the advanced ones. |
| `pxx` | Set Panning | `panning` | No | Channel panning 0 left to 15 right. Values not divisible by 4 are rounded down when saved. |
| `cxx` | Volume Slide Up | `volslideup` | No | Like `Ax0`, but parameter is the per-tick step. |
| `dxx` | Volume Slide Down | `volslidedown` | No | Like `A0y`. |
| `axx` | Fine Volume Slide Up | `finevolup` | No | Like `EAx`, first tick only. |
| `bxx` | Fine Volume Slide Down | `finevoldown` | No | Like `EBx`, first tick only. |
| `gxx` | Tone Portamento | `toneportamento` | Yes | Like `3xx`, but 16× coarser (`g01` = `310`). Combining it with `3xx` doubles the parameter and ignores `3xx`. Ineffective beside `EDx`. |
| `hxx` | Vibrato Depth | `vibratodepth` | Yes | Vibrato depth `xx`, speed taken from the last `4xy` or `uxx`. |
| `uxx` | Vibrato Speed | `vibratospeed` | No | Sets vibrato speed without starting a vibrato. |
| `lxx` | Panning Slide Left | `panslideleft` | No | Like `P0y`. |
| `rxx` | Panning Slide Right | `panslideright` | No | Like `Px0`. |

## Authoring notes

- **Two IDs share the letter `F`.** `context.format.effect_commands` contains separate `speed` and `tempo` entries; choose by the value (below 32 → `speed`, 32+ → `tempo`).
- **Whole families share one ID.** `E1x`–`EEx` all use `modcmdex`; `X1x`, `X2x`, `X5x`, `X6x`, `X9x`, `XAx` all use `xfineportaupdown`. The sub-command is the high nibble of `effect_parameter`.
- **The volume column is separate from `Cxx`.** `vxx` and the other volume commands change only the current note; `Cxx` writes the note volume from the effect column. A cell can carry both.
- **Global commands change playback flow, not just the cell.** `Bxx`, `Dxx`, `E6x`, `EEx`, `Fxx`, `Gxx`, `Hxy` and `X6x` affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **Effect memory** applies to the commands marked *Yes*: a `00`/`000` parameter repeats the last non-zero value written.
- The Pattern tools cannot verify audio. A written command is only as correct as the catalog entry and the range documented above.
