# `release_occupancy`

End a retained read-only session.

```json
{"session": "opaque-token"}
```

Use this only when the candidate still equals the baseline. If edits exist, the tool returns `candidateExists`; choose `handoff_for_review` or `abort_session` instead.

After `ok: true`, the token is invalid and there is no proposal for the human to review. A waiting Pattern switch is resolved by the human decision, a saved preference, force release, disconnect, or document close.

Completion criterion: `ok: true` is returned and no later call reuses the token.
