# S3M effect column and volume column

Self-contained reference for authoring the S3M effect column and volume column (`.s3m`) through the Pattern MCP. Everything needed is in this file; S3M uses a command set entirely different from MOD and XM, so do not carry commands over from another format's file.

## Before you write

1. Read `context.format.effect_commands` and `context.format.volume_commands` for the bound document. Each lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Pick a command by its `name`, then write that entry's numeric `id` into `effect_command` / `volume_command` and the value into `effect_parameter` / `volume`.
2. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.
3. The catalog is OpenMPT's full S3M set. Commands marked *non-ST3* below are not part of original Scream Tracker 3, but OpenMPT still exposes and plays them. The published catalog, not this list, is authoritative.

## How to read these tables

- The effect column is written through `effect_command` (the catalog `id` of the entry named in the table) and `effect_parameter`; the volume column through `volume_command` (again a catalog `id`) and `volume`.
- `effect_parameter` is hexadecimal. `xx` is a whole byte (two hex digits), `xy` is two independent nibbles (`x` = high nibble, `y` = low nibble), and `x` is a single digit. It holds the plain integer value of the digits: a parameter written `05` is 5, `20` is 32, `8F` is 143.
- `volume` is decimal, matching the range the tool publishes.
- A command family shown as a bitwise expression shares one catalog entry. Write that entry's `id` and combine the sub-command into the parameter, for example `s3mcmdex` with `effect_parameter` = `0x90 + x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Stay inside the documented ranges.

## Frequency units

Linear frequency slides are not available in S3M, so one unit of a pitch slide (Portamento Down, Portamento Up, Tone Portamento) is one *period*, a metric inverse to frequency: the lower the note, the smaller the audible change. A period is 3,546,895 ÷ frequency. Extra-fine units are 4× finer than fine units.

## Effect parameter tables

### Vibrato / tremolo / panbrello waveform (Set Vibrato Waveform, Set Tremolo Waveform, Set Panbrello Waveform)

| Parameter | Waveform |
| --- | --- |
| 0 (default) | Sine, retriggered on each new note |
| 1 | Sawtooth, retriggered |
| 2 | Square, retriggered |
| 3 | Random, retriggered — avoid |

Each waveform is 64 points long; the speed parameter advances by that many points per tick.

### Retrigger volume (Retrigger, the high nibble)

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

### Sound control (Sound Control, the low nibble)

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

| Name | `effect_command` | `effect_parameter` | Mem | Description |
| --- | --- | --- | --- | --- |
| Set Speed | `speed` | `xx` | No | Sets the module Speed (ticks per row). |
| Position Jump | `positionjump` | `xx` | — | Jumps to Order position `xx`; parameter `00` restarts. On the same row as Pattern Break, it selects the Pattern that Pattern Break breaks into. |
| Pattern Break | `patternbreak` | `xx` | — | Jumps to row `xx` of the next Pattern. Range `00`–`3F`; higher values are ignored. |
| Volume Slide / Fine Volume Slide | `volumeslide` | `xy` | Global | Parameter `0y` down by `y`, `x0` up by `x`, every tick except the first; `Fy`/`xF` apply only on the first tick. Volume caps at 64. |
| Portamento Down / Fine / Extra Fine | `portamentodown` | `xx` | Global | Parameter `xx` slides down every tick except the first; `Fx` on the first tick only; `Ex` at 4× the precision of `Fx`. |
| Portamento Up / Fine / Extra Fine | `portamentoup` | `xx` | Global | Parameter `xx` slides up every tick except the first; `Fx` on the first tick only; `Ex` at 4× the precision of `Fx`. |
| Tone Portamento | `toneportamento` | `xx` | Yes | Slides the previous note toward the current note by `xx` per tick except the first. |
| Vibrato | `vibrato` | `xy` | Yes | Vibrato, speed `x`, depth `y`, waveform from Set Vibrato Waveform. Shares memory with Fine Vibrato. |
| Tremor | `tremor` | `xy` | Global | Volume on for `x` ticks, off for `y` ticks. |
| Arpeggio | `arpeggio` | `xy` | Global | Cycles within one row between the current note, +`x` semitones, +`y` semitones. |
| Volume Slide + Vibrato | `vibratovol` | `xy` | Global | Volume Slide plus vibrato at parameter `00`. |
| Volume Slide + Tone Portamento | `toneportavol` | `xy` | Global | Volume Slide plus tone portamento at parameter `00`. |
| Set Channel Volume | `channelvolume` | `xx` | — | *non-ST3* Channel volume multiplier, `00` off to `40` full. |
| Channel Volume Slide | `channelvolslide` | `xy` | Yes | *non-ST3* Like Volume Slide, applied to channel volume. |
| Sample Offset | `offset` | `xx` | Yes | Starts the sample at `xx × 256`. Requires a note in the same cell. |
| Panning Slide / Fine Panning Slide | `panningslide` | `xy` | Yes | *non-ST3* Parameter `0y` slides right by `y`, `x0` slides left by `x`, every tick except the first; `Fy`/`xF` apply on the first tick only. |
| Retrigger | `retrig` | `xy` | Global | Retriggers every `y` ticks and changes volume by the high nibble (see the retrigger volume table). |
| Tremolo | `tremolo` | `xy` | Global | Volume tremolo, speed `x`, depth `y`, waveform from Set Tremolo Waveform. |
| Glissando Control | `s3mcmdex` | `0x10 + x` | — | `0x10` off, `0x11` on. Quirky and not widely supported. |
| Set Finetune | `s3mcmdex` | `0x20 + x` | — | Legacy command; overrides the current sample's C-5 frequency with a MOD finetune value. |
| Set Vibrato Waveform | `s3mcmdex` | `0x30 + x` | — | Selects the waveform table above for later Vibrato commands. |
| Set Tremolo Waveform | `s3mcmdex` | `0x40 + x` | — | Selects the waveform table above for later Tremolo commands. |
| Set Panbrello Waveform | `s3mcmdex` | `0x50 + x` | — | *non-ST3* Selects the waveform table above for later Panbrello commands. |
| Fine Pattern Delay | `s3mcmdex` | `0x60 + x` | — | Extends the row by `x` ticks; multiple Fine Pattern Delay commands on a row sum. |
| Set Panning (coarse) | `s3mcmdex` | `0x80 + x` | — | Coarse panning, `0` left to `F` right. Effect-column Set Panning is finer. |
| Sound Control | `s3mcmdex` | `0x90 + x` | — | *non-ST3* Runs a sound control command (see the table above). |
| High Offset | `s3mcmdex` | `0xA0 + x` | — | *non-ST3* Adds `x × 65536` to all following Sample Offset parameters. |
| Pattern Loop Start | `s3mcmdex` | `0xB0` | — | Marks the Pattern Loop start. |
| Pattern Loop | `s3mcmdex` | `0xB0 + x` | — | Jumps back to the Pattern Loop Start row until `x` jumps total. Cannot span Patterns. Range `1`–`F`. |
| Note Cut | `s3mcmdex` | `0xC0 + x` | — | Stops the sample after `x` ticks; ignored if `x` is 0 or ≥ Speed. |
| Note Delay | `s3mcmdex` | `0xD0 + x` | — | Delays the cell's note/instrument by `x` ticks; ignored if `x` is 0 or ≥ Speed. |
| Pattern Delay | `s3mcmdex` | `0xE0 + x` | — | Repeats the row `x` times without retriggering notes; only the leftmost Pattern Delay counts. |
| Decrease Tempo | `tempo` | `0x00 + x` | Yes | Lowers Tempo by `x` BPM on every tick except the first. |
| Increase Tempo | `tempo` | `0x10 + x` | Yes | Raises Tempo by `x` BPM on every tick except the first. |
| Set Tempo | `tempo` | `xx` | No | Sets Tempo when `xx` ≥ `20`. |
| Fine Vibrato | `finevibrato` | `xy` | Yes | Like Vibrato with 4× precision. Shares memory with Vibrato. |
| Set Global Volume | `globalvolume` | `xx` | — | Song global volume, `00` off to `40` full. |
| Global Volume Slide | `globalvolslide` | `xy` | Yes | *non-ST3* Like Volume Slide, applied to the global volume. |
| Set Panning | `panning8` | `xx` | — | Channel panning, `00` left to `80` right. Parameter `A4` (a non-ST3 extension) enables surround on the channel; any other Set Panning value on that channel disables it. |
| Panbrello | `panbrello` | `xy` | Yes | *non-ST3* Panning oscillates with speed `x`, depth `y`, waveform from Set Panbrello Waveform. |
| MIDI Macro | `midi` | `xx` | — | *non-ST3* Runs a MIDI macro. S3M files do not store macros, so only the default macro configuration applies, and the Pattern tools cannot read or change it — the audible result is not verifiable here. |

The base and extended catalogs both expose a `tempo` command ID that carries Decrease Tempo, Increase Tempo and Set Tempo; the sub-command is the high nibble of `effect_parameter`. The base catalog also contains an internal `dummy` entry that is never useful to write.

## Volume column commands

All parameters are decimal, matching the published range. S3M's volume column is intentionally small.

| Name | `volume_command` | `volume` | Mem | Description |
| --- | --- | --- | --- | --- |
| Set Volume | `volume` | `0`–`64` | No | Sets note volume, 0 off to 64 full. |
| Set Panning | `panning` | `0`–`64` | No | Channel panning, 0 left to 64 right. *non-ST3* |

Volume slides, fine slides, vibrato depth, tone portamento and panning slides that other formats allow in the volume column are **not** available in S3M. Express them in the effect column (Volume Slide, Tone Portamento, Vibrato, Panning Slide).

## Authoring notes

- **Whole families share one ID.** Glissando Control through Pattern Delay all use `s3mcmdex`; Decrease Tempo, Increase Tempo and Set Tempo all use `tempo`. The sub-command is the high nibble of `effect_parameter`.
- **S3M uses global effect memory.** Commands marked **Global** above recall any previous non-zero parameter written in the same column, not just their own. This differs from MOD/XM/IT and is the most common source of unintended slides.
- **Global commands change playback flow, not just the cell.** Position Jump, Pattern Break, Pattern Loop, Pattern Delay, Set Speed, Set Tempo, Set Global Volume, Global Volume Slide and Fine Pattern Delay affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **AdLib / OPL3 instruments** are supported in S3M, but Sample Offset, Sound Control, High Offset and Set Panning parameter `A4` have no effect on them, Set Channel Volume and Channel Volume Slide work only with them in OpenMPT, and effect-column Set Panning and volume-column Set Panning collapse to hard left, center and hard right.
- The Pattern tools cannot verify audio. A written command is only as correct as the catalog entry and the range documented above.
