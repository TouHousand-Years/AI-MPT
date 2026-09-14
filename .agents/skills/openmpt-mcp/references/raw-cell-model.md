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

Use each returned cell's `note_kind` and `note_name` as the semantic reading. Preserve plugin-control cells byte-for-byte. For note cuts, fades, and other unsupported special notes, preserve the note and instrument; their ordinary volume and effect columns remain independently editable when the format supports the requested commands.

## Volume and effects

- `context.format.volume_commands` lists every writable `volume_command` ID, semantic name, and inclusive `parameter_min` / `parameter_max` range for the bound module format.
- `context.format.effect_commands` does the same for `effect_command`; non-empty effect commands accept the full raw parameter byte range published there.
- The meaning behind each catalogued command is format-specific. Read the file for `context.format.name` before choosing one: [effects-mod.md](effects-mod.md), [effects-xm.md](effects-xm.md), [effects-s3m.md](effects-s3m.md), [effects-it.md](effects-it.md), [effects-mptm.md](effects-mptm.md).
- An empty column requires both fields to be zero: `volume_command: 0, volume: 0` or `effect_command: 0, effect_parameter: 0`.
- Preserve any existing command that is absent from its format catalog, or replace it with a catalogued command. Never invent an unlisted command ID.

## Sparse reads versus sparse writes

`context.cells` omits completely empty cells. In `replace_pattern_segment`, omitted rows inside the requested segment are deliberately replaced by six zeroes. Therefore, before replacing a segment that contains existing events, fetch its current candidate range and include every non-empty row that must survive.

Completion criterion: every emitted replacement cell has six fields, every authored command and parameter is in the current format catalog, protected values match the candidate, and every omitted row is intentionally cleared.
