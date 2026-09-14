# `get_pattern_context`

Read the sparse semantic Score Context of the bound Pattern.

## Arguments

All arguments are optional:

```json
{
  "session": "opaque-token",
  "occupy": true,
  "baseline": false,
  "range": {
    "first_row": 0,
    "row_count": 16,
    "first_channel": 0,
    "channel_count": 4
  }
}
```

- Omit `session` for a new read; set `occupy: true` when later calls are needed.
- Pass the retained `session` to read its candidate; `baseline: true` instead reads the immutable original snapshot.
- Omit `range` for the whole Pattern. Each count must be positive and the rectangle must remain within `context.rows` and `context.channels`.

## Read the result

On success, inspect `context.pattern`, dimensions, resolved `range`, sparse `cells`, `timing`, `format`, `instruments`, and `samples`. Each non-empty cell includes semantic names plus the authoritative six-field `raw` value. Before authoring volume or effect columns, read `context.format.volume_commands` and `context.format.effect_commands`; each entry supplies the writable raw command `id`, semantic `name`, and inclusive parameter bounds for this module format. To learn what a catalogued command does in this format, read the matching file for `context.format.name`: [effects-mod.md](effects-mod.md), [effects-xm.md](effects-xm.md), [effects-s3m.md](effects-s3m.md), [effects-it.md](effects-it.md), [effects-mptm.md](effects-mptm.md). `context.pattern` is the binding, not the UI cursor: manual navigation never changes it, and only an approved [`switch_pattern`](switch-pattern.md) rebinds it.

A retained success also returns `session`. Store it exactly and follow the mandatory ending in [session-lifecycle.md](session-lifecycle.md).

Completion criterion: the requested rectangle and musical facts are accounted for; retained reads remain open only when a subsequent call is immediately required.
