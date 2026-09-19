# `reorder_pattern_order`

Rebuild the current Sequence while preserving every existing entry. Valid Patterns listed by `get_pattern_order.unreferenced_patterns` may also be inserted.

Acquire a retained session, read `get_pattern_order`, then pass the current Order indices in their desired destination order:

```json
{
  "session": "opaque-token",
  "order": [2, 0, 1, 3]
}
```

`order` must be a complete permutation of the zero-based `entries[].order` values: every current index exactly once. The source entry moves as a unit, so duplicate Pattern references, `skip`, `stop`, and invalid entries are preserved. Re-read Order after acquiring occupancy so the permutation describes the occupied document state.

To insert an unreferenced Pattern at an exact destination, add a `{ "pattern": N }` item at that position while retaining every original Order index exactly once:

```json
{
  "session": "opaque-token",
  "order": [2, {"pattern": 7}, 0, 1, 3]
}
```

`N` is a zero-based Pattern number, not an Order index. It must identify an existing valid Pattern that the current Sequence does not reference, and the same Pattern may be inserted only once per call. To move or duplicate an already referenced Pattern, use its existing Order occurrence; the insertion form deliberately cannot create another reference to it. The resulting length must fit the module format's Order limit.

The operation applies immediately, preserves the bound Pattern and token, remaps playback and restart Order positions to the same source entries, refreshes the retention timeout, and returns `status: "reordered"` with the resulting `entries`. Insertions are also summarized in `inserted_patterns`. The identity permutation without insertions returns `status: "unchanged"`. Validation failure changes nothing.

Completion criterion: the returned `entries` match the requested permutation and insertions, and the returned session token is retained for the next call or ended exactly once.
