# `handoff_for_review`

Freeze the complete candidate as one immutable single-Pattern proposal and release occupancy atomically.

```json
{"session": "opaque-token"}
```

Call this only after candidate verification and after confirming there is at least one intended difference from the baseline. The full proposal `diff` (`row`, `channel`, `before`, `after`) is guaranteed only on the manual `pending_review` result; an automatic `applied` result and a failed automatic attempt carry no `diff`. Finish verification before the call and keep the verified scope from your own reads.

## Read the returned status

Occupancy has ended either way; the returned status decides what happened to the proposal.

- `status: "applied"` (`ok: true`): the saved **Always accept submissions** preference was on and the frozen proposal passed the atomic apply path, including revision check, cell validation, and native Undo preparation. The live document changed and no `diff` is returned; report it as applied.
- `status: "pending_review"` (`ok: true`): the default manual path. The proposal waits for the human and the live document is unchanged. Report the diff scope and that review is pending.
- `ok: false` with `status: "pending_review"`: an automatic application attempt failed (for example `stale`, `emptyProposal`, or `commitFailed`). The document is unchanged and the proposal is retained for the human, but occupancy has ended. Report the failure code; do not retry or describe the proposal as applied.

Never promise that automatic acceptance will succeed: the preference removes the human click, not the validation and Undo gates. If the human turns **Always accept submissions** on while a proposal is already waiting, the app attempts that proposal once with the same statuses.

Completion criterion: the returned `status` is read and reported accurately, and the user knows whether the live document changed or a proposal still waits.
