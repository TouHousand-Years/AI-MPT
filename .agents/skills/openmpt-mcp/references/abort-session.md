# `abort_session`

Discard the complete private candidate and release occupancy atomically.

```json
{"session": "opaque-token"}
```

Use this when the requested edit should not be proposed, the plan is no longer safe, the user asks to cancel, or a retained session cannot be completed. This discards every accumulated candidate edit, not only the latest call.

After `ok: true`, the token is invalid. State that the candidate was discarded and the live document was not changed.

Completion criterion: `ok: true` is returned and no later call reuses the token.
