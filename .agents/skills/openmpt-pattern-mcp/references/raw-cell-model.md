# Raw Pattern cell model

Pattern coordinates are zero-based. Each cell has six byte-valued fields:

```json
{
  "note": 61,
  "instrument": 1,
  "volume_command": 0,
  "volume": 0,
  "effect_command": 0,
  "effect_parameter": 0
}
```

Every supplied replacement cell must contain all six fields, each from 0 through 255.

## Notes and instruments

- `note: 0` is empty.
- Pitched notes use OpenMPT numbering: `1` is C-0 and `61` is C-5 (middle C / MIDI 60). For ordinary 12-tone notes, raw note = MIDI note + 1.
- `note: 255` is note-off and is writable only when `context.format.note_off` is true.
- Create pitched notes only inside `context.format.note_min` through `note_max`.
- A nonzero instrument must name an existing entry in `context.instruments`; when there are no instruments, it may refer to an existing `context.samples` entry.

Use each returned cell's `note_kind` and `note_name` as the semantic reading. Preserve plugin-control notes, note cuts, fades, and other unsupported special-note cells byte-for-byte.

## Volume and effects

- The ordinary volume command is raw `volume_command: 1`, with `volume` from 0 through `context.format.volume_max` (currently at most 64).
- An empty volume column is `volume_command: 0, volume: 0`.
- Preserve every other volume command byte-for-byte.
- Preserve `effect_command` and `effect_parameter` byte-for-byte; this MCP slice does not author effects.

## Sparse reads versus sparse writes

`context.cells` omits completely empty cells. In `replace_pattern_segment`, omitted rows inside the requested segment are deliberately replaced by six zeroes. Therefore, before replacing a segment that contains existing events, fetch its current candidate range and include every non-empty row that must survive.

Completion criterion: every emitted replacement cell has six fields, all protected fields match the candidate, and every omitted row is intentionally cleared.
