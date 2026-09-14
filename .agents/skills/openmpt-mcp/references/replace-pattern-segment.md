# `replace_pattern_segment`

Replace one contiguous row segment in one channel of the retained private candidate.

## Arguments

```json
{
  "session": "opaque-token",
  "channel": 1,
  "first_row": 0,
  "row_count": 4,
  "cells": [
    {
      "row": 0,
      "cell": {
        "note": 61,
        "instrument": 1,
        "volume_command": 0,
        "volume": 0,
        "effect_command": 0,
        "effect_parameter": 0
      }
    }
  ]
}
```

`cells[].row` is an absolute Pattern row inside `[first_row, first_row + row_count)`. Rows must be unique. Omitted rows in the segment become empty cells.

## Safe construction

1. Read the current candidate over the exact segment.
2. Start from its six-field raw cells.
3. Change only supported note and instrument fields or commands listed in `context.format.volume_commands` and `context.format.effect_commands`; keep each parameter inside its published inclusive range.
4. Include every non-empty row that must remain; omit only rows intended to become empty.
5. Prefer the smallest musically coherent segment. Several calls accumulate atomically in the same candidate.

A request outside the Pattern selection captured at session start requires human expansion approval: the call waits and returns the final resolution—the normal replacement result when approved, or `rangeRejected` when declined. `pending_approval` is an app-internal marker, never a tool result. Do not issue a concurrent retry; a second call while approval waits is refused with `approvalPending`. If the owner rejects it, remain within the granted envelope or abort.

After the approved switch, the target's whole Pattern (all rows and channels) replaces the initial selection as the granted envelope. Accumulated candidate edits block a Pattern switch (`candidateExists`); finish them with handoff or abort first.

On success, `cells` reports the resulting segment and `diff` reports changes made by this call. An empty `diff` is a no-op.

Completion criterion: re-read the candidate with the same session and verify every intended changed and preserved row before another write or handoff.
