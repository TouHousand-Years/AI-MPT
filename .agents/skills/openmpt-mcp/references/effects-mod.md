# MOD effect column

Self-contained reference for authoring the MOD effect column (`.mod`) through the Pattern MCP. MOD has **no volume column**, so every volume-related move is made in the effect column. Command IDs and meanings are reassigned between formats; do not carry them over from another format's file.

## Before you write

1. Read `context.format.effect_commands` for the bound document. It lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Pick a command by its `name`, then write that entry's numeric `id` into `effect_command` and the value into `effect_parameter`.
2. MOD publishes **no** volume commands (`context.format.volume_commands` contains only `none`). A nonzero `volume_command` is rejected. Keep `volume_command: 0, volume: 0`.
3. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.

## How to read these tables

- The effect column is written through two fields: `effect_command`, which takes the catalog `id` of the entry named in the table, and `effect_parameter`.
- `effect_parameter` is hexadecimal. `xx` is a whole byte (two hex digits), `xy` is two independent nibbles (`x` = high nibble, `y` = low nibble), and `x` is a single digit.
- It holds the plain integer value of the digits: a parameter written `05` is 5, `20` is 32, `7D` is 125.
- A command family shown as a bitwise expression shares one catalog entry. Write that entry's `id` and combine the sub-command into the parameter, for example `modcmdex` with `effect_parameter` = `0x10 + x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Values outside the documented range are not recommended, because other players may read them differently.

## Frequency units

With the default MOD mixing, one unit of a pitch slide (Portamento Up, Portamento Down, Tone Portamento) is one *period*, a metric inverse to frequency: the lower the note, the smaller the audible change. Linear frequency slides are not available in MOD.

## Effect parameter tables

### Vibrato / tremolo waveform (Set Vibrato Waveform, Set Tremolo Waveform)

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

| Name | `effect_command` | `effect_parameter` | Mem | Description |
| --- | --- | --- | --- | --- |
| Arpeggio | `arpeggio` | `xy` | No | Cycles within one row between the current note, current note + `x` semitones, and current note + `y` semitones. |
| Portamento Up | `portamentoup` | `xx` | No | Raises pitch by `xx` units on every tick except the first. |
| Portamento Down | `portamentodown` | `xx` | No | Lowers pitch by `xx` units on every tick except the first. |
| Tone Portamento | `toneportamento` | `xx` | Yes | Slides the previous note's pitch toward the current note by `xx` units on every tick except the first. |
| Vibrato | `vibrato` | `xy` | Yes | Vibrato with speed `x`, depth `y`, using the waveform set by Set Vibrato Waveform. |
| Volume Slide + Tone Portamento | `toneportavol` | `xy` | No | Volume Slide combined with tone portamento at parameter `00`. |
| Volume Slide + Vibrato | `vibratovol` | `xy` | No | Volume Slide combined with vibrato at parameter `00`. |
| Tremolo | `tremolo` | `xy` | Yes | Volume tremolo with speed `x`, depth `y`, using the waveform set by Set Tremolo Waveform. |
| Set Panning | `panning8` | `xx` | — | Channel panning, `00` hard left to `FF` hard right. |
| Sample Offset | `offset` | `xx` | Yes | Starts the sample at `xx × 256` samples. Has no effect unless the same cell also contains a note. |
| Volume Slide | `volumeslide` | `xy` | No | Parameter `0y` lowers note volume by `y` per tick; `x0` raises it by `x` per tick. Both on every tick except the first. |
| Position Jump | `positionjump` | `xx` | — | Jumps to Order position `xx`; parameter `00` restarts from the first Order. Range `00`–`7F`. On the same row as Pattern Break, it selects the Pattern that Pattern Break breaks into. |
| Set Volume | `volume` | `xx` | — | Sets the current note volume, `00` off to `40` full. |
| Pattern Break | `patternbreak` | `xx` | — | Jumps to row `xx` of the next Order's Pattern. Range `00`–`3F`. |
| Set Filter | `modcmdex` | `0x00 + x` | — | Amiga LED lowpass filter: `0x00` enables, `0x01` disables. Only audible when the Amiga resampler is active. Leave off unless explicitly required. |
| Fine Portamento Up | `modcmdex` | `0x10 + x` | No | Like Portamento Up, applied only on the first tick of the row. |
| Fine Portamento Down | `modcmdex` | `0x20 + x` | No | Like Portamento Down, applied only on the first tick of the row. |
| Glissando Control | `modcmdex` | `0x30 + x` | — | `0x30` disables, `0x31` enables semitone-quantized tone portamento. Quirky and not widely supported. |
| Set Vibrato Waveform | `modcmdex` | `0x40 + x` | — | Selects the waveform table above for later Vibrato commands. |
| Set Finetune | `modcmdex` | `0x50 + x` | — | Temporarily overrides the playing note's finetune; only works when the same cell holds a note. |
| Pattern Loop Start | `modcmdex` | `0x60` | — | Marks the row used as the loop start for Pattern Loop. |
| Pattern Loop | `modcmdex` | `0x60 + x` | — | Jumps back to the Pattern Loop Start row until `x` total jumps have happened. Loops cannot span Patterns. Range `1`–`F`. |
| Set Tremolo Waveform | `modcmdex` | `0x70 + x` | — | Selects the waveform table above for later Tremolo commands. |
| Set Panning (coarse) | `modcmdex` | `0x80 + x` | — | Coarse channel panning, `0` left to `F` right. |
| Retrigger | `modcmdex` | `0x90 + x` | No | Retriggers the note every `x` ticks. |
| Fine Volume Slide Up | `modcmdex` | `0xA0 + x` | No | Like the Volume Slide up-slide, applied only on the first tick of the row. |
| Fine Volume Slide Down | `modcmdex` | `0xB0 + x` | No | Like the Volume Slide down-slide, applied only on the first tick of the row. |
| Note Cut | `modcmdex` | `0xC0 + x` | — | Sets note volume to 0 after `x` ticks; ignored if `x` is at least the current Speed. |
| Note Delay | `modcmdex` | `0xD0 + x` | — | Delays the cell's note or instrument change by `x` ticks; if `x` is at least the current Speed, nothing in the cell plays. |
| Pattern Delay | `modcmdex` | `0xE0 + x` | — | Repeats the current row `x` times without retriggering notes; effects still run. Only the rightmost Pattern Delay on a row counts. |
| Invert Loop | `modcmdex` | `0xF0 + x` | — | **Destructive.** Walks the sample loop and inverts sample points at speed `x`; the change is applied to the sample data at playback and cannot be undone automatically. `0xF0` cancels it. Avoid. |
| Set Speed / Tempo | `speed` for `xx` < `20`, `tempo` for `xx` ≥ `20` | `xx` | — | `xx` < `20` sets Speed (ticks per row); `xx` ≥ `20` sets Tempo. The catalog lists `speed` and `tempo` as two separate IDs that OpenMPT displays in the same column, so pick the ID that matches the value. Avoid `20` and `00`. |

## Authoring notes

- **Two entries share one display slot.** `context.format.effect_commands` contains separate `speed` and `tempo` entries. For a value below 32 use the `speed` ID; for 32 and above use the `tempo` ID.
- **The `modcmdex` family is one command ID.** Set Filter through Invert Loop all use the single `modcmdex` ID; the sub-command lives in the high nibble of `effect_parameter`.
- **Global commands change playback flow, not just the cell.** Position Jump, Pattern Break, Pattern Loop, Pattern Delay and Set Speed / Tempo affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **Volume must go through the effect column** (`volume`, `volumeslide` and the `modcmdex` fine-volume sub-commands) because MOD has no volume column.
- **Effects with effect memory** (Tone Portamento, Vibrato, Tremolo, Sample Offset) continue a previous slide or oscillation when written with parameter `00`/`000`.
- The Pattern tools cannot verify audio. A written effect is only as correct as the catalog entry and the range documented above.
