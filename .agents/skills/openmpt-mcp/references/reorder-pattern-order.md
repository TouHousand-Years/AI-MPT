# `reorder_pattern_order`

Reorder every entry in the current Sequence without inserting, deleting, or rewriting any entry.

Acquire a retained session, read `get_pattern_order`, then pass the current Order indices in their desired destination order:

```json
{
  "session": "opaque-token",
  "order": [2, 0, 1, 3]
}
```

`order` must be a complete permutation of the zero-based `entries[].order` values: every current index exactly once. The source entry moves as a unit, so duplicate Pattern references, `skip`, `stop`, and invalid entries are preserved. Re-read Order after acquiring occupancy so the permutation describes the occupied document state.

The operation applies immediately, preserves the bound Pattern and token, remaps playback and restart Order positions to the same source entries, refreshes the retention timeout, and returns `status: "reordered"` with the resulting `entries`. The identity permutation returns `status: "unchanged"`. Validation failure changes nothing.

Completion criterion: the returned `entries` match the requested permutation and the returned session token is retained for the next call or ended exactly once.
