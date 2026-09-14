# XM effect column and volume column

Self-contained reference for authoring the XM effect column and volume column (`.xm`) through the Pattern MCP. Everything needed is in this file; command IDs are reassigned between formats, so do not consult another format's file.

## Before you write

1. Read `context.format.effect_commands` and `context.format.volume_commands` for the bound document. Each lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Pick a command by its `name`, then write that entry's numeric `id` into `effect_command` / `volume_command` and the value into `effect_parameter` / `volume`.
2. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.
3. The catalog is OpenMPT's full XM set. Commands marked *hack* below are not implemented by Fasttracker II, but OpenMPT still exposes and plays them. The published catalog, not this list, is authoritative.

## How to read these tables

- The effect column is written through `effect_command` (the catalog `id` of the entry named in the table) and `effect_parameter`; the volume column through `volume_command` (again a catalog `id`) and `volume`.
- `effect_parameter` is hexadecimal. `xx` is a whole byte (two hex digits), `xy` is two independent nibbles (`x` = high nibble, `y` = low nibble), and `x` is a single digit. It holds the plain integer value of the digits: a parameter written `05` is 5, `20` is 32, `0F` is 15.
- `volume` is decimal, matching the range the tool publishes.
- A command family shown as a bitwise expression shares one catalog entry. Write that entry's `id` and combine the sub-command into the parameter, for example `xfineportaupdown` with `effect_parameter` = `0x90 + x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Stay inside the documented ranges.

## Frequency units

XM uses linear frequency slides by default, so one unit of Portamento Up, Portamento Down or Tone Portamento is 1/16 semitone and one unit of Fine Portamento Up or Fine Portamento Down is 1/64 semitone. If linear slides are disabled, a unit is one *period*, computed as 3,579,364 ÷ frequency, and lower notes change less than higher notes.

## Effect parameter tables

### Vibrato / tremolo / panbrello waveform (Set Vibrato Waveform, Set Tremolo Waveform, Set Panbrello Waveform)

| Parameter | Waveform | Parameter | Waveform |
| --- | --- | --- | --- |
| 0 (default) | Sine, retriggered | 4 | Sine, continue |
| 1 | Sawtooth, retriggered | 5 | Sawtooth, continue |
| 2 | Square, retriggered | 6 | Square, continue |
| 3 | Random, retriggered — avoid | 7 | Random, continue — avoid |

"Retrigger" restarts the waveform on each new note; "continue" resumes from the last position and is exclusive to MOD and XM. Each waveform is 64 points long; speed is how many points per tick are advanced.

### Retrigger volume (Retrigger, the high nibble)

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

### Sound control (Sound Control, the low nibble)

| Parameter | Name | Description |
| --- | --- | --- |
| 0 / 1 | Surround Off / On | Enables or disables surround playback on the channel. |
| 8 / 9 | Reverb Off / On | Per-channel reverb. Discouraged; prefer a reverb plugin. |
| A / B | Center / Quad Surround | Switches the global surround mode. |
| C / D | Global / Local Filters | When a resonant filter is active, global filters persist until explicitly reset; local filters revert on the next note. |
| E / F | Play Forward / Backward | Forces the current sample's playback direction. |

## Effect column commands

All parameters are hexadecimal. "Mem" is effect memory at parameter 0: **Yes** recalls the command's own last non-zero parameter, **No** does nothing, **—** means zero has its own literal meaning. Entries marked *hack* are ModPlug extensions that Fasttracker II does not implement.

| Name | `effect_command` | `effect_parameter` | Mem | Description |
| --- | --- | --- | --- | --- |
| Arpeggio | `arpeggio` | `xy` | No | Cycles within one row between the current note, +`x` semitones, +`y` semitones. |
| Portamento Up | `portamentoup` | `xx` | Yes | Raises pitch by `xx` on every tick except the first. |
| Portamento Down | `portamentodown` | `xx` | Yes | Lowers pitch by `xx` on every tick except the first. |
| Tone Portamento | `toneportamento` | `xx` | Yes | Slides the previous note toward the current note by `xx` per tick except the first. |
| Vibrato | `vibrato` | `xy` | Yes | Vibrato, speed `x`, depth `y`, waveform from Set Vibrato Waveform. |
| Volume Slide + Tone Portamento | `toneportavol` | `xy` | Yes | Volume Slide combined with tone portamento at parameter `00`. |
| Volume Slide + Vibrato | `vibratovol` | `xy` | Yes | Volume Slide combined with vibrato at parameter `00`. |
| Tremolo | `tremolo` | `xy` | Yes | Volume tremolo, speed `x`, depth `y`, waveform from Set Tremolo Waveform. |
| Set Panning | `panning8` | `xx` | — | Sample panning, `00` left to `FF` right. A later instrument-column entry resets it to the sample default. |
| Sample Offset | `offset` | `xx` | Yes | Starts the sample at `xx × 256`. Requires a note in the same cell. |
| Volume Slide | `volumeslide` | `xy` | Yes | Parameter `0y` down by `y`, `x0` up by `x`, every tick except the first. |
| Position Jump | `positionjump` | `xx` | — | Jumps to Order position `xx`. When Pattern Break is to its right on the same row, it selects the Pattern that Pattern Break breaks into. |
| Set Volume | `volume` | `xx` | — | Note volume, `00` off to `40` full. |
| Pattern Break | `patternbreak` | `xx` | — | Jumps to row `xx` of the next Pattern. Keep `xx` ≤ `3F` for Fasttracker II compatibility. |
| Fine Portamento Up | `modcmdex` | `0x10 + x` | Yes | Like Portamento Up, first tick only. |
| Fine Portamento Down | `modcmdex` | `0x20 + x` | Yes | Like Portamento Down, first tick only. |
| Glissando Control | `modcmdex` | `0x30 + x` | — | `0x30` off, `0x31` on. Quirky and not widely supported. |
| Set Vibrato Waveform | `modcmdex` | `0x40 + x` | — | Selects the waveform table above for later Vibrato commands. |
| Set Finetune | `modcmdex` | `0x50 + x` | — | Temporary finetune override; only works with a note in the same cell. |
| Pattern Loop Start | `modcmdex` | `0x60` | — | Marks the Pattern Loop start. A Fasttracker II bug also makes the following Pattern start from that row unless a Pattern Break with parameter `00` closes the current Pattern. |
| Pattern Loop | `modcmdex` | `0x60 + x` | — | Jumps back to the Pattern Loop Start row until `x` jumps total. Cannot span Patterns. Range `1`–`F`. |
| Set Tremolo Waveform | `modcmdex` | `0x70 + x` | — | Selects the waveform table above for later Tremolo commands. |
| Set Panning (coarse) | `modcmdex` | `0x80 + x` | — | Coarse panning, `0` left to `F` right. |
| Retrigger | `modcmdex` | `0x90 + x` | No | Retriggers the note every `x` ticks. |
| Fine Volume Slide Up | `modcmdex` | `0xA0 + x` | Yes | Like the Volume Slide up-slide, first tick only. |
| Fine Volume Slide Down | `modcmdex` | `0xB0 + x` | Yes | Like the Volume Slide down-slide, first tick only. |
| Note Cut | `modcmdex` | `0xC0 + x` | — | Volume 0 after `x` ticks; ignored if `x` ≥ Speed. |
| Note Delay | `modcmdex` | `0xD0 + x` | — | Delays the cell's note/instrument by `x` ticks. Buggy in Fasttracker II (portamento beside it is ignored); do not rely on the emulation. |
| Pattern Delay | `modcmdex` | `0xE0 + x` | — | Repeats the row `x` times without retriggering notes; only the rightmost Pattern Delay counts. |
| Set Active Macro | `modcmdex` | `0xF0 + x` | — | *hack* Selects the channel's active parametered macro. |
| Set Speed / Tempo | `speed` for `xx` < `20`, `tempo` for `xx` ≥ `20` | `xx` | — | `xx` < `20` sets Speed; `xx` ≥ `20` sets Tempo. The catalog lists `speed` and `tempo` as two IDs that OpenMPT displays in the same column; pick the ID matching the value. Avoid `00`. |
| Set Global Volume | `globalvolume` | `xx` | — | Song global volume, `00` off to `40` full. |
| Global Volume Slide | `globalvolslide` | `xy` | Yes | Like Volume Slide, applied to the global volume. |
| Key Off | `keyoff` | `xx` | — | Triggers Note Off after `xx` ticks. Avoid `00`, which interferes with other entries in the cell. |
| Set Envelope Position | `setenvposition` | `xx` | — | Sets the volume envelope position to `xx` ticks; also moves the panning envelope when its sustain point is enabled. |
| Panning Slide | `panningslide` | `xy` | Yes | Parameter `0y` slides left by `y`, `x0` slides right by `x`, first tick only. |
| Retrigger | `retrig` | `xy` | Yes | Retriggers every `y` ticks and changes volume by the high nibble (see the retrigger volume table). Buggy with a volume command in the same cell. |
| Tremor | `tremor` | `xy` | Yes | Volume on for `x`+1 ticks, off for `y`+1, every tick except the first. |
| Extra Fine Portamento Up | `xfineportaupdown` | `0x10 + x` | Yes | Like Fine Portamento Up with 4× precision. |
| Extra Fine Portamento Down | `xfineportaupdown` | `0x20 + x` | Yes | Like Fine Portamento Down with 4× precision. |
| Set Panbrello Waveform | `xfineportaupdown` | `0x50 + x` | — | *hack* Selects the waveform table above for later Panbrello commands. |
| Fine Pattern Delay | `xfineportaupdown` | `0x60 + x` | — | *hack* Extends the row by `x` ticks; multiple Fine Pattern Delay commands on a row sum. |
| Sound Control | `xfineportaupdown` | `0x90 + x` | — | *hack* Runs a sound control command (see the table above). |
| High Offset | `xfineportaupdown` | `0xA0 + x` | — | *hack* Adds `x × 65536` to all following Sample Offset parameters. |
| Panbrello | `panbrello` | `xy` | Yes | *hack* Panning oscillates with speed `x`, depth `y`, waveform from Set Panbrello Waveform. |
| MIDI Macro | `midi` | `xx` | — | *hack* Runs a MIDI macro. The macro itself lives in module configuration the Pattern tools cannot read or change, so the audible result is not verifiable here. |
| Smooth MIDI Macro | `smoothmidi` | `xx` | — | *hack* As MIDI Macro, interpolated over the row. |
| Parameter Extension | `xparam` | `xx` | — | *hack* Extends the parameter of a preceding Position Jump, Pattern Break, Sample Offset or Set Tempo command by combining bytes. |

The catalog may also list an internal `dummy` command; it is an import placeholder and is never useful to write.

## Volume column commands

All parameters are decimal, and the tool publishes the exact range. In XM the maximum is 64 for Set Volume and 15 for Set Panning and every other volume command.

| Name | `volume_command` | `volume` | Mem | Description |
| --- | --- | --- | --- | --- |
| Set Volume | `volume` | `0`–`64` | No | Sets note volume, 0 off to 64 full. The plain volume command; the remaining commands below are the advanced ones. |
| Set Panning | `panning` | `0`–`15` | No | Channel panning 0 left to 15 right. Values not divisible by 4 are rounded down when saved. |
| Volume Slide Up | `volslideup` | `0`–`15` | No | Like the Volume Slide up-slide, but the parameter is the per-tick step. |
| Volume Slide Down | `volslidedown` | `0`–`15` | No | Like the Volume Slide down-slide. |
| Fine Volume Slide Up | `finevolup` | `0`–`15` | No | Like Fine Volume Slide Up in the effect column, first tick only. |
| Fine Volume Slide Down | `finevoldown` | `0`–`15` | No | Like Fine Volume Slide Down in the effect column, first tick only. |
| Tone Portamento | `toneportamento` | `0`–`15` | Yes | Like the effect-column Tone Portamento, but 16× coarser (parameter 1 equals effect-column parameter 16). Combining it with the effect-column command doubles the parameter and ignores the effect-column one. Ineffective beside Note Delay. |
| Vibrato Depth | `vibratodepth` | `0`–`15` | Yes | Vibrato depth `xx`, speed taken from the last Vibrato or Vibrato Speed command. |
| Vibrato Speed | `vibratospeed` | `0`–`15` | No | Sets vibrato speed without starting a vibrato. |
| Panning Slide Left | `panslideleft` | `0`–`15` | No | Like the Panning Slide left-slide. |
| Panning Slide Right | `panslideright` | `0`–`15` | No | Like the Panning Slide right-slide. |

## Authoring notes

- **Two entries share one display slot.** `context.format.effect_commands` contains separate `speed` and `tempo` entries; choose by the value (below 32 → `speed`, 32+ → `tempo`).
- **Whole families share one ID.** Fine Portamento Up through Pattern Delay all use `modcmdex`; Extra Fine Portamento Up, Extra Fine Portamento Down, Set Panbrello Waveform, Fine Pattern Delay, Sound Control and High Offset all use `xfineportaupdown`. The sub-command is the high nibble of `effect_parameter`.
- **The volume column is separate from the effect-column Set Volume.** The volume-column Set Volume and the other volume commands change only the current note; `volume` in the effect column writes the note volume. A cell can carry both.
- **Global commands change playback flow, not just the cell.** Position Jump, Pattern Break, Pattern Loop, Pattern Delay, Set Speed / Tempo, Set Global Volume, Global Volume Slide and Fine Pattern Delay affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **Effect memory** applies to the commands marked *Yes*: a `00`/`000` parameter repeats the last non-zero value written.
- The Pattern tools cannot verify audio. A written command is only as correct as the catalog entry and the range documented above.
