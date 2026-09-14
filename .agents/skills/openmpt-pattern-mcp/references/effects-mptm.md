# MPTM effect column and volume column

Self-contained reference for authoring the MPTM effect column and volume column (`.mptm`) through the Pattern MCP. Everything needed is in this file; MPTM expands the IT command set, so do not consult another format's file.

## Before you write

1. Read `context.format.effect_commands` and `context.format.volume_commands` for the bound document. Each lists the exact writable commands: numeric `id`, semantic `name`, inclusive `parameter_min` / `parameter_max`. Use those IDs in `effect_command` / `volume_command`; the letters below are only the tracker display notation.
2. Read the current candidate over the segment you are about to replace, and include every row that must survive. Omitted rows become empty cells.
3. The published catalog, not this list, is authoritative. If a command is absent from it, do not invent an ID for it.

## How to read the notation

- An effect is one letter plus a parameter, written `Axy`. Uppercase letters address the effect column; lowercase letters address the volume column.
- `xx` is a two-digit hexadecimal value read as one byte; `xy` is two independent hexadecimal nibbles. Volume column parameters are decimal, matching the range the tool publishes.
- In the raw cell, `effect_parameter` holds the plain integer value of the digits: `D05` → 5, `D20` → 32, `H8F` → 143. `volume` holds the decimal volume-column value.
- A single command family shares one raw command. Write the catalog ID for the base letter and put the whole parameter in the field: `S9x` → the `s3mcmdex` ID with `effect_parameter` = `0x90 | x`.
- The tool accepts any byte 0–255 in `effect_parameter`, but the format only interprets some values. Stay inside the documented ranges.

## Frequency units

MPTM uses linear frequency slides by default, so one unit of `Exx`, `Fxx`, `Gxx` is 1/16 semitone, one unit of `EFx`, `FFx` is 1/64 semitone, and `EEx`/`FEx` are a further 4× finer than the fine variants. If linear slides are disabled, a unit is one *period*: a metric inverse to frequency, where lower notes change less than higher notes.

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
| C / D | Global / Local Filters | Global filters persist until explicitly reset with a maximum cutoff and minimum resonance; local filters revert on the next note. |
| E / F | Play Forward / Backward | Forces the current sample's playback direction. |

In MPTM all of these are freely usable and are not considered hacks.

## Effect column commands

All parameters are hexadecimal. "Mem" is effect memory at parameter 0: **Yes** recalls the command's own last non-zero parameter, **No** does nothing, **—** means zero has its own literal meaning.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `Axx` | Set Speed | `speed` | No | Sets the module Speed (ticks per row). |
| `Bxx` | Position Jump | `positionjump` | — | Jumps to Order position `xx`; `B00` restarts. On the same row as `Cxx`, `Bxx` selects the Pattern `Cxx` breaks into. |
| `Cxx` | Pattern Break | `patternbreak` | — | Jumps to row `xx` of the next Pattern; a value past that Pattern's length is treated as `00`. |
| `Dxy` | Volume Slide / Fine Volume Slide | `volumeslide` | Yes | `D0y` down by `y`, `Dx0` up by `x`, every tick except the first; `DFy`/`DxF` apply on the first tick only. A `D0F` volume-down uses every tick. Volume caps at 64. |
| `Exx` | Portamento Down / Fine / Extra Fine | `portamentodown` | Yes | `Exx` every tick except the first; `EFx` on the first tick; `EEx` at 4× the precision of `EFx`. |
| `Fxx` | Portamento Up / Fine / Extra Fine | `portamentoup` | Yes | `Fxx` every tick except the first; `FFx` on the first tick; `FEx` at 4× the precision of `FFx`. |
| `Gxx` | Tone Portamento | `toneportamento` | Yes | Slides the previous note toward the current note by `xx` per tick except the first. |
| `Hxy` | Vibrato | `vibrato` | Yes | Vibrato, speed `x`, depth `y`, waveform from `S3x`. Shares memory with `Uxy`. |
| `Ixy` | Tremor | `tremor` | Yes | Volume on for `x` ticks, off for `y` ticks. On instrument plugins it sends note-on/note-off instead of changing volume. |
| `Jxy` | Arpeggio | `arpeggio` | Yes | Cycles within one row between the current note, +`x` semitones, +`y` semitones. |
| `Kxy` | Volume Slide + Vibrato | `vibratovol` | Yes | `Dxy` volume slide plus `H00`. |
| `Lxy` | Volume Slide + Tone Portamento | `toneportavol` | Yes | `Dxy` volume slide plus `G00`. |
| `Mxx` | Set Channel Volume | `channelvolume` | — | Channel volume multiplier, `00` off to `40` full. |
| `Nxy` | Channel Volume Slide | `channelvolslide` | Yes | Like `Dxy`, applied to channel volume. |
| `Oxx` | Sample Offset | `offset` | Yes | Starts the sample at `xx × 256`. Requires a note in the same cell. Combine with the volume-column `o0x` for the percentage and cue-point behaviours below. |
| `Pxy` | Panning Slide / Fine Panning Slide | `panningslide` | Yes | `P0y` slides right by `y`, `Px0` slides left by `x`, every tick except the first; `PFy`/`PxF` apply on the first tick only. |
| `Qxy` | Retrigger | `retrig` | Yes | Retriggers every `y` ticks and changes volume by the `x` nibble (see the retrigger volume table). |
| `Rxy` | Tremolo | `tremolo` | Yes | Volume tremolo, speed `x`, depth `y`, waveform from `S4x`. |
| `S1x` | Glissando Control | `s3mcmdex` | — | `S10` off, `S11` on. Quirky and not widely supported. |
| `S2x` | Set Finetune | `s3mcmdex` | — | Legacy command; overrides the current sample's C-5 frequency with a MOD finetune value. |
| `S3x` | Set Vibrato Waveform | `s3mcmdex` | — | Selects the waveform table above for later `Hxy`. |
| `S4x` | Set Tremolo Waveform | `s3mcmdex` | — | Selects the waveform table above for later `Rxy`. |
| `S5x` | Set Panbrello Waveform | `s3mcmdex` | — | Selects the waveform table above for later `Yxy`. |
| `S6x` | Fine Pattern Delay | `s3mcmdex` | — | Extends the row by `x` ticks; multiple `S6x` on a row sum. |
| `S70` | Past Note Cut | `s3mcmdex` | — | Cuts all notes ringing from New Note Actions on this channel. |
| `S71` | Past Note Off | `s3mcmdex` | — | Sends Note Off to those notes. |
| `S72` | Past Note Fade | `s3mcmdex` | — | Fades out those notes. |
| `S73` | NNA Note Cut | `s3mcmdex` | — | Sets the active note's New Note Action to Note Cut. |
| `S74` | NNA Note Continue | `s3mcmdex` | — | Sets it to Continue. |
| `S75` | NNA Note Off | `s3mcmdex` | — | Sets it to Note Off. |
| `S76` | NNA Note Fade | `s3mcmdex` | — | Sets it to Note Fade. |
| `S77` | Volume Envelope Off | `s3mcmdex` | — | Disables the active note's volume envelope. |
| `S78` | Volume Envelope On | `s3mcmdex` | — | Enables it. |
| `S79` | Panning Envelope Off | `s3mcmdex` | — | Disables the active note's panning envelope. |
| `S7A` | Panning Envelope On | `s3mcmdex` | — | Enables it. |
| `S7B` | Pitch Envelope Off | `s3mcmdex` | — | Disables the active note's pitch or filter envelope. |
| `S7C` | Pitch Envelope On | `s3mcmdex` | — | Enables the active note's pitch envelope. |
| `S7D` | Force Pitch Envelope | `s3mcmdex` | — | Enables the envelope and forces it to act as a pitch envelope. |
| `S7E` | Force Filter Envelope | `s3mcmdex` | — | Enables the envelope and forces it to act as a filter cutoff envelope. |
| `S8x` | Set Panning | `s3mcmdex` | — | Coarse panning, `0` left to `F` right. `Xxx` is finer. |
| `S9x` | Sound Control | `s3mcmdex` | — | Runs a sound control command (see the table above). |
| `SAx` | High Offset | `s3mcmdex` | — | Adds `x × 65536` to all following `Oxx` offsets. |
| `SB0` | Pattern Loop Start | `s3mcmdex` | — | Marks the `SBx` loop start. |
| `SBx` | Pattern Loop | `s3mcmdex` | — | Jumps back to the `SB0` row until `x` jumps total. Cannot span Patterns. Range `1`–`F`. |
| `SCx` | Note Cut | `s3mcmdex` | — | Stops the sample after `x` ticks; ignored if `x` ≥ Speed, and `x` = 0 is treated as 1. |
| `SDx` | Note Delay | `s3mcmdex` | — | Delays the cell's note/instrument by `x` ticks; ignored if `x` ≥ Speed, and `x` = 0 is treated as 1. |
| `SEx` | Pattern Delay | `s3mcmdex` | — | Repeats the row `x` times without retriggering notes; only the leftmost `SEx` counts. |
| `SFx` | Set Active Macro | `s3mcmdex` | — | Selects the channel's active parametered macro. |
| `T0x` | Decrease Tempo | `tempo` | Yes | Lowers Tempo by `x` BPM on every tick except the first. |
| `T1x` | Increase Tempo | `tempo` | Yes | Raises Tempo by `x` BPM on every tick except the first. |
| `Txx` | Set Tempo | `tempo` | No | Sets Tempo when `xx` ≥ `20`. |
| `Uxy` | Fine Vibrato | `finevibrato` | Yes | Like `Hxy` with 4× precision. Shares memory with `Hxy`. |
| `Vxx` | Set Global Volume | `globalvolume` | — | Song global volume, `00` off to `80` full. |
| `Wxy` | Global Volume Slide | `globalvolslide` | Yes | Like `Dxy`, applied to the global volume. |
| `Xxx` | Set Panning | `panning8` | — | Channel panning, `00` left to `FF` right. |
| `Yxy` | Panbrello | `panbrello` | Yes | Panning oscillates with speed `x`, depth `y`, waveform from `S5x`. |
| `Zxx` | MIDI Macro | `midi` | — | Runs a MIDI macro. The macro itself lives in module configuration the Pattern tools cannot read or change, so the audible result is not verifiable here. |
| `\xx` | Smooth MIDI Macro | `smoothmidi` | — | As `Zxx`, interpolated over the row. |
| `:xy` | Note Delay + Cut | `delaycut` | — | Delays the cell's note by `x` ticks and cuts it after `x + y` ticks. If `x` ≥ Speed the note is ignored entirely; if `x + y` ≥ Speed only the cut is ignored. |
| `#xx` | Parameter Extension | `xparam` | — | Extends the parameter of the preceding `Bxx`, `Cxx`, `Oxx`, `Txx`, `+xx` or `*xx`. One `#xx` below such a command combines as `original × 256 + xx`; for `Oxx` up to four rows combine, weighting `1`, `0x100`, `0x10000`, … from the bottom up. |
| `+xx` | Finetune | `finetune` | — | Changes the current note's tuning; `+80` is centre, lower values flatten, higher values sharpen. Depth is ±1 semitone in sample mode, the instrument's pitch-bend range for sample instruments, and the plugin/device pitch-bend depth for plugins. Extend with `#xx` for finer control. |
| `*xx` | Finetune (Smooth) | `finetune_smooth` | — | Exactly `+xx`, but slides from the previous tuning to the new value over the row. |

The `S` family and the `T` family each share one raw command ID (`s3mcmdex` and `tempo`); the sub-command is the high nibble of `effect_parameter`. The catalog also contains an internal `dummy` entry that is never useful to write.

## Volume column commands

All parameters are decimal, matching the published range: 64 for `vxx` and `pxx`, 9 for every other command.

| Eff | Name | Catalog `name` | Mem | Description |
| --- | --- | --- | --- | --- |
| `vxx` | Set Volume | `volume` | — | Sets note volume, 0 off to 64 full. |
| `pxx` | Set Panning | `panning` | — | Channel panning, 0 left to 64 right. |
| `o0x` | Sample Cue | `offset` | Yes | Starts the sample from cue point `x` instead of position 0. Requires a note in the same cell. Shares memory with `Oxx` and can be combined with it (see below). |
| `a0x` | Fine Volume Slide Up | `finevolup` | Yes | Like `DxF`, first tick only. |
| `b0x` | Fine Volume Slide Down | `finevoldown` | Yes | Like `DFy`, first tick only. |
| `c0x` | Volume Slide Up | `volslideup` | Yes | Like `Dx0`, every tick except the first. |
| `d0x` | Volume Slide Down | `volslidedown` | Yes | Like `D0y`, every tick except the first. |
| `e0x` | Portamento Down | `portadown` | Yes | Like `Exx`, but 4× coarser (`e01` = `E04`). |
| `f0x` | Portamento Up | `portaup` | Yes | Like `Fxx`, but 4× coarser (`f01` = `F04`). |
| `g0x` | Tone Portamento | `toneportamento` | Yes | Like `Gxx`. Parameter mapping: `g00`→`G00`, `g01`→`G01`, `g02`→`G04`, `g03`→`G08`, `g04`→`G10`, `g05`→`G20`, `g06`→`G40`, `g07`→`G60`, `g08`→`G80`, `g09`→`GFF`. |
| `h0x` | Vibrato Depth | `vibratodepth` | Yes | Vibrato depth `x`, speed from the last `Hxy` or `Uxy`. |

The XM vibrato-speed volume command (`u`, `vibratospeed`) is *not* among MPTM's MCP-editable volume commands; set vibrato speed with `Hxy`/`Uxy` in the effect column instead.

### Combining `o0x` with `Oxx`

- `o00` + `Oxx`: the effect-column offset becomes a percentage of total sample length, `xx × 1/256`. `o00` + `O80` plays the second half of any sample. Add `#xx` for more precision.
- `o0x` (x > 0) + `Oyy`: `Oyy` is added on top of the cue point, so `o05` + `O01` plays from the 5th cue point plus 256 samples.

## Not editable through the Pattern tools

MPTM also supports **Parameter Control Events** (automating plugin parameters). Those are stored as special PC cells. The Pattern tools preserve PC cells byte-for-byte and refuse to change them, so they can be read around but never authored here. Do not attempt to encode them as ordinary effect cells.

## Authoring notes

- **Whole families share one ID.** `S1x`–`SFx` all use `s3mcmdex`; `T0x`, `T1x` and `Txx` all use `tempo`. The sub-command is the high nibble of `effect_parameter`.
- **Global commands change playback flow, not just the cell.** `Bxx`, `Cxx`, `SBx`, `SEx`, `Axx`, `Txx`, `Vxx`, `Wxy`, `S6x` and the `S7x` NNA changes affect the whole song. Use them deliberately; their audible result is not verifiable from the Pattern tools alone.
- **`#xx`, `+xx` and `*xx` are parameter modifiers, not standalone effects.** `#xx` extends the command on the row(s) above it; `+xx`/`*xx` tune the note in the same cell.
- **Instrument-plugin caveats.** `Ixy` sends note-on/off to plugins instead of changing volume, and many volume and panning effects act on samples only, so avoid them on plugin channels.
- **AdLib / OPL3 instruments** are supported in MPTM, but `Oxx`, `o0x`, `SAx` and all `S9x` parameters other than `C`/`D` (global/local filters) have no effect on them, and `Xxx`/`pxx` collapse to hard left, center and hard right. Avoid `Pxy`/`Yxy` with them; the reduced granularity makes the slides unpredictable.
- The Pattern tools cannot verify audio. A written command is only as correct as the catalog entry and the range documented above.
