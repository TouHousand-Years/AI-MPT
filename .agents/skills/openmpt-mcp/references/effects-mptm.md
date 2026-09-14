# MPTM effect column and volume column

Self-contained reference for authoring the MPTM effect column and volume column (`.mptm`) through the Pattern MCP. Everything needed is in this file; MPTM expands the IT command set, so do not consult another format's file.

## Before you write

1. Read `context.format.effect_commands` and `context.format.volume_commands` for the bound document. Each lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Pick a command by its `name`, then write that entry's numeric `id` into `effect_command` / `volume_command` and the value into `effect_parameter` / `volume`.
2. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.
3. The published catalog, not this list, is authoritative. If a command is absent from it, do not invent an ID for it.

## How to read these tables

- The effect column is written through `effect_command` (the catalog `id` of the entry named in the table) and `effect_parameter`; the volume column through `volume_command` (again a catalog `id`) and `volume`.
- `effect_parameter` is hexadecimal. `xx` is a whole byte (two hex digits), `xy` is two independent nibbles (`x` = high nibble, `y` = low nibble), and `x` is a single digit. It holds the plain integer value of the digits: a parameter written `05` is 5, `20` is 32, `8F` is 143.
- `volume` is decimal, matching the range the tool publishes.
- A command family shown as a bitwise expression shares one catalog entry. Write that entry's `id` and combine the sub-command into the parameter, for example `s3mcmdex` with `effect_parameter` = `0x90 + x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Stay inside the documented ranges.

## Frequency units

MPTM uses linear frequency slides by default, so one unit of Portamento Down, Portamento Up or Tone Portamento is 1/16 semitone, one unit of their fine variants (parameter `Fx`) is 1/64 semitone, and the extra-fine variants (parameter `Ex`) are a further 4× finer than the fine variants. If linear slides are disabled, a unit is one *period*: a metric inverse to frequency, where lower notes change less than higher notes.

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
| C / D | Global / Local Filters | Global filters persist until explicitly reset with a maximum cutoff and minimum resonance; local filters revert on the next note. |
| E / F | Play Forward / Backward | Forces the current sample's playback direction. |

In MPTM all of these are freely usable and are not considered hacks.

## Effect column commands

All parameters are hexadecimal. "Mem" is effect memory at parameter 0: **Yes** recalls the command's own last non-zero parameter, **No** does nothing, **—** means zero has its own literal meaning.

| Name | `effect_command` | `effect_parameter` | Mem | Description |
| --- | --- | --- | --- | --- |
| Set Speed | `speed` | `xx` | No | Sets the module Speed (ticks per row). |
| Position Jump | `positionjump` | `xx` | — | Jumps to Order position `xx`; parameter `00` restarts. On the same row as Pattern Break, it selects the Pattern that Pattern Break breaks into. |
| Pattern Break | `patternbreak` | `xx` | — | Jumps to row `xx` of the next Pattern; a value past that Pattern's length is treated as `00`. |
| Volume Slide / Fine Volume Slide | `volumeslide` | `xy` | Yes | Parameter `0y` down by `y`, `x0` up by `x`, every tick except the first; `Fy`/`xF` apply on the first tick only. A `0F` volume-down uses every tick. Volume caps at 64. |
| Portamento Down / Fine / Extra Fine | `portamentodown` | `xx` | Yes | Parameter `xx` every tick except the first; `Fx` on the first tick; `Ex` at 4× the precision of `Fx`. |
| Portamento Up / Fine / Extra Fine | `portamentoup` | `xx` | Yes | Parameter `xx` every tick except the first; `Fx` on the first tick; `Ex` at 4× the precision of `Fx`. |
| Tone Portamento | `toneportamento` | `xx` | Yes | Slides the previous note toward the current note by `xx` per tick except the first. |
| Vibrato | `vibrato` | `xy` | Yes | Vibrato, speed `x`, depth `y`, waveform from Set Vibrato Waveform. Shares memory with Fine Vibrato. |
| Tremor | `tremor` | `xy` | Yes | Volume on for `x` ticks, off for `y` ticks. On instrument plugins it sends note-on/note-off instead of changing volume. |
| Arpeggio | `arpeggio` | `xy` | Yes | Cycles within one row between the current note, +`x` semitones, +`y` semitones. |
| Volume Slide + Vibrato | `vibratovol` | `xy` | Yes | Volume Slide plus vibrato at parameter `00`. |
| Volume Slide + Tone Portamento | `toneportavol` | `xy` | Yes | Volume Slide plus tone portamento at parameter `00`. |
| Set Channel Volume | `channelvolume` | `xx` | — | Channel volume multiplier, `00` off to `40` full. |
| Channel Volume Slide | `channelvolslide` | `xy` | Yes | Like Volume Slide, applied to channel volume. |
| Sample Offset | `offset` | `xx` | Yes | Starts the sample at `xx × 256`. Requires a note in the same cell. Combine with the volume-column Sample Cue command for the percentage and cue-point behaviours below. |
| Panning Slide / Fine Panning Slide | `panningslide` | `xy` | Yes | Parameter `0y` slides right by `y`, `x0` slides left by `x`, every tick except the first; `Fy`/`xF` apply on the first tick only. |
| Retrigger | `retrig` | `xy` | Yes | Retriggers every `y` ticks and changes volume by the high nibble (see the retrigger volume table). |
| Tremolo | `tremolo` | `xy` | Yes | Volume tremolo, speed `x`, depth `y`, waveform from Set Tremolo Waveform. |
| Glissando Control | `s3mcmdex` | `0x10 + x` | — | `0x10` off, `0x11` on. Quirky and not widely supported. |
| Set Finetune | `s3mcmdex` | `0x20 + x` | — | Legacy command; overrides the current sample's C-5 frequency with a MOD finetune value. |
| Set Vibrato Waveform | `s3mcmdex` | `0x30 + x` | — | Selects the waveform table above for later Vibrato commands. |
| Set Tremolo Waveform | `s3mcmdex` | `0x40 + x` | — | Selects the waveform table above for later Tremolo commands. |
| Set Panbrello Waveform | `s3mcmdex` | `0x50 + x` | — | Selects the waveform table above for later Panbrello commands. |
| Fine Pattern Delay | `s3mcmdex` | `0x60 + x` | — | Extends the row by `x` ticks; multiple Fine Pattern Delay commands on a row sum. |
| Past Note Cut | `s3mcmdex` | `0x70` | — | Cuts all notes ringing from New Note Actions on this channel. |
| Past Note Off | `s3mcmdex` | `0x71` | — | Sends Note Off to those notes. |
| Past Note Fade | `s3mcmdex` | `0x72` | — | Fades out those notes. |
| NNA Note Cut | `s3mcmdex` | `0x73` | — | Sets the active note's New Note Action to Note Cut. |
| NNA Note Continue | `s3mcmdex` | `0x74` | — | Sets it to Continue. |
| NNA Note Off | `s3mcmdex` | `0x75` | — | Sets it to Note Off. |
| NNA Note Fade | `s3mcmdex` | `0x76` | — | Sets it to Note Fade. |
| Volume Envelope Off | `s3mcmdex` | `0x77` | — | Disables the active note's volume envelope. |
| Volume Envelope On | `s3mcmdex` | `0x78` | — | Enables it. |
| Panning Envelope Off | `s3mcmdex` | `0x79` | — | Disables the active note's panning envelope. |
| Panning Envelope On | `s3mcmdex` | `0x7A` | — | Enables it. |
| Pitch Envelope Off | `s3mcmdex` | `0x7B` | — | Disables the active note's pitch or filter envelope. |
| Pitch Envelope On | `s3mcmdex` | `0x7C` | — | Enables the active note's pitch envelope. |
| Force Pitch Envelope | `s3mcmdex` | `0x7D` | — | Enables the envelope and forces it to act as a pitch envelope. |
| Force Filter Envelope | `s3mcmdex` | `0x7E` | — | Enables the envelope and forces it to act as a filter cutoff envelope. |
| Set Panning (coarse) | `s3mcmdex` | `0x80 + x` | — | Coarse panning, `0` left to `F` right. Effect-column Set Panning is finer. |
| Sound Control | `s3mcmdex` | `0x90 + x` | — | Runs a sound control command (see the table above). |
| High Offset | `s3mcmdex` | `0xA0 + x` | — | Adds `x × 65536` to all following Sample Offset parameters. |
| Pattern Loop Start | `s3mcmdex` | `0xB0` | — | Marks the Pattern Loop start. |
| Pattern Loop | `s3mcmdex` | `0xB0 + x` | — | Jumps back to the Pattern Loop Start row until `x` jumps total. Cannot span Patterns. Range `1`–`F`. |
| Note Cut | `s3mcmdex` | `0xC0 + x` | — | Stops the sample after `x` ticks; ignored if `x` ≥ Speed, and `x` = 0 is treated as 1. |
| Note Delay | `s3mcmdex` | `0xD0 + x` | — | Delays the cell's note/instrument by `x` ticks; ignored if `x` ≥ Speed, and `x` = 0 is treated as 1. |
| Pattern Delay | `s3mcmdex` | `0xE0 + x` | — | Repeats the row `x` times without retriggering notes; only the leftmost Pattern Delay counts. |
| Set Active Macro | `s3mcmdex` | `0xF0 + x` | — | Selects the channel's active parametered macro. |
| Decrease Tempo | `tempo` | `0x00 + x` | Yes | Lowers Tempo by `x` BPM on every tick except the first. |
| Increase Tempo | `tempo` | `0x10 + x` | Yes | Raises Tempo by `x` BPM on every tick except the first. |
| Set Tempo | `tempo` | `xx` | No | Sets Tempo when `xx` ≥ `20`. |
| Fine Vibrato | `finevibrato` | `xy` | Yes | Like Vibrato with 4× precision. Shares memory with Vibrato. |
| Set Global Volume | `globalvolume` | `xx` | — | Song global volume, `00` off to `80` full. |
| Global Volume Slide | `globalvolslide` | `xy` | Yes | Like Volume Slide, applied to the global volume. |
| Set Panning | `panning8` | `xx` | — | Channel panning, `00` left to `FF` right. |
| Panbrello | `panbrello` | `xy` | Yes | Panning oscillates with speed `x`, depth `y`, waveform from Set Panbrello Waveform. |
| MIDI Macro | `midi` | `xx` | — | Runs a MIDI macro. The macro itself lives in module configuration the Pattern tools cannot read or change, so the audible result is not verifiable here. |
| Smooth MIDI Macro | `smoothmidi` | `xx` | — | As MIDI Macro, interpolated over the row. |
| Note Delay + Cut | `delaycut` | `xy` | — | Delays the cell's note by `x` ticks and cuts it after `x + y` ticks. If `x` ≥ Speed the note is ignored entirely; if `x + y` ≥ Speed only the cut is ignored. |
| Parameter Extension | `xparam` | `xx` | — | Extends the parameter of the preceding Position Jump, Pattern Break, Sample Offset, Set Tempo, Finetune or Finetune (Smooth) command. One Parameter Extension below such a command combines as `original × 256 + xx`; for Sample Offset up to four rows combine, weighting `1`, `0x100`, `0x10000`, … from the bottom up. |
| Finetune | `finetune` | `xx` | — | Changes the current note's tuning; parameter `80` is centre, lower values flatten, higher values sharpen. Depth is ±1 semitone in sample mode, the instrument's pitch-bend range for sample instruments, and the plugin/device pitch-bend depth for plugins. Extend with Parameter Extension for finer control. |
| Finetune (Smooth) | `finetune_smooth` | `xx` | — | Exactly Finetune, but slides from the previous tuning to the new value over the row. |

The `s3mcmdex` family and the `tempo` family each share one catalog ID; the sub-command is the high nibble of `effect_parameter`. The catalog also contains an internal `dummy` entry that is never useful to write.

## Volume column commands

All parameters are decimal, matching the published range: 0–64 for Set Volume and Set Panning, 0–9 for every other volume command.

| Name | `volume_command` | `volume` | Mem | Description |
| --- | --- | --- | --- | --- |
| Set Volume | `volume` | `0`–`64` | — | Sets note volume, 0 off to 64 full. |
| Set Panning | `panning` | `0`–`64` | — | Channel panning, 0 left to 64 right. |
| Sample Cue | `offset` | `0`–`9` | Yes | Starts the sample from cue point `x` instead of position 0. Requires a note in the same cell. Shares memory with the effect-column Sample Offset and can be combined with it (see below). |
| Fine Volume Slide Up | `finevolup` | `0`–`9` | Yes | Like the effect-column Fine Volume Slide Up, first tick only. |
| Fine Volume Slide Down | `finevoldown` | `0`–`9` | Yes | Like the effect-column Fine Volume Slide Down, first tick only. |
| Volume Slide Up | `volslideup` | `0`–`9` | Yes | Like the Volume Slide up-slide, every tick except the first. |
| Volume Slide Down | `volslidedown` | `0`–`9` | Yes | Like the Volume Slide down-slide, every tick except the first. |
| Portamento Down | `portadown` | `0`–`9` | Yes | Like Portamento Down, but 4× coarser (parameter 1 equals effect-column parameter 4). |
| Portamento Up | `portaup` | `0`–`9` | Yes | Like Portamento Up, but 4× coarser (parameter 1 equals effect-column parameter 4). |
| Tone Portamento | `toneportamento` | `0`–`9` | Yes | Like the effect-column Tone Portamento. Parameter mapping to the effect-column parameter: 0→`00`, 1→`01`, 2→`04`, 3→`08`, 4→`10`, 5→`20`, 6→`40`, 7→`60`, 8→`80`, 9→`FF`. |
| Vibrato Depth | `vibratodepth` | `0`–`9` | Yes | Vibrato depth `x`, speed from the last Vibrato or Fine Vibrato command. |

The XM vibrato-speed volume command (`vibratospeed`) is *not* among MPTM's MCP-editable volume commands; set vibrato speed with Vibrato or Fine Vibrato in the effect column instead.

### Combining Sample Cue with Sample Offset

- Sample Cue value 0 + Sample Offset `xx`: the effect-column offset becomes a percentage of total sample length, `xx × 1/256`. Sample Cue 0 + Sample Offset `80` plays the second half of any sample. Add Parameter Extension for more precision.
- Sample Cue value `x` > 0 + Sample Offset `yy`: the offset is added on top of the cue point, so Sample Cue 5 + Sample Offset `01` plays from the 5th cue point plus 256 samples.

## Not editable through the Pattern tools

MPTM also supports **Parameter Control Events** (automating plugin parameters). Those are stored as special PC cells. The Pattern tools preserve PC cells byte-for-byte and refuse to change them, so they can be read around but never authored here. Do not attempt to encode them as ordinary effect cells.

## Authoring notes

- **Whole families share one ID.** Glissando Control through Set Active Macro all use `s3mcmdex`; Decrease Tempo, Increase Tempo and Set Tempo all use `tempo`. The sub-command is the high nibble of `effect_parameter`.
- **Global commands change playback flow, not just the cell.** Position Jump, Pattern Break, Pattern Loop, Pattern Delay, Set Speed, Set Tempo, Set Global Volume, Global Volume Slide, Fine Pattern Delay and the Past Note / NNA changes affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **Parameter Extension, Finetune and Finetune (Smooth) are parameter modifiers, not standalone effects.** Parameter Extension extends the command on the row(s) above it; Finetune and Finetune (Smooth) tune the note in the same cell.
- **Instrument-plugin caveats.** Tremor sends note-on/off to plugins instead of changing volume, and many volume and panning effects act on samples only, so avoid them on plugin channels.
- **AdLib / OPL3 instruments** are supported in MPTM, but Sample Offset, volume-column Sample Cue, High Offset and all Sound Control parameters other than `C`/`D` (global/local filters) have no effect on them, and effect-column Set Panning and volume-column Set Panning collapse to hard left, center and hard right. Avoid Panning Slide and Panbrello with them; the reduced granularity makes the slides unpredictable.
- The Pattern tools cannot verify audio. A written command is only as correct as the catalog entry and the range documented above.
