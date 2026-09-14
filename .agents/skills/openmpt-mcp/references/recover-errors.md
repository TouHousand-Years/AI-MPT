# Recover from typed failures

Every tool result is structured. Branch on `error.layer` and `error.code`; use `reason` and any `row`, `field`, or `value` diagnostics. A failed replacement is atomic: it does not partially change the candidate.

## Attachment and transport

- `notAttached`: follow [connect-target.md](connect-target.md), then make a fresh read.
- `instanceGone`: the outcome of the last mutation may be uncertain. Do not replay it. Reconnect explicitly, read fresh state, and let the observed state determine the next action.
- `schemaFailure`: correct malformed arguments when they are locally knowable. If it describes the target file or application response, ask the human to republish the document before a fresh read.

## Session state

- `busy`: another retained session or frozen proposal owns the capability. Wait for its owner or human review; do not attempt to steal occupancy. A frozen proposal also blocks `switch_pattern` until it is applied or rejected.
- `approvalPending`: expansion and Pattern switching share this code. It answers only a concurrent second call while an earlier call waits for the human; the waiting call itself resolves to its final result (approved mutation or `rangeRejected`/`patternSwitchRejected`). Wait for the human; do not send another mutation.
- `patternSwitchRejected`: the human declined the waiting switch. The original binding is unchanged; a retained session keeps its token, while a session-less request was released. Continue on the bound Pattern or abort.
- `occupancyLost`: treat the token and private candidate as gone. Acquire fresh context and re-plan from observed state.
- `stale`: document dependencies changed and the session was released. Acquire fresh context and re-plan; never replay the old mutation list blindly.
- `rangeRejected`: continue only within the granted envelope, or abort the session.
- `candidateExists`: finish edited work with handoff or abort instead of releasing as read-only; the same ending clears the block on `switch_pattern`.

## Request validation

- `validationFailure`: fix the reported range or field using fresh candidate context and the rules in [raw-cell-model.md](raw-cell-model.md). For `switch_pattern`, choose an existing target from [get-pattern-order.md](get-pattern-order.md), or a missing index within the module format limit when Order has capacity; the current binding is unchanged. Retry only the corrected bounded call with the still-valid token.
- `boundPatternViolation`: another tool carried a Pattern index. Use `switch_pattern` to change the binding, and operate only on the Pattern bound at session acquisition.
- `unsupported`: the requested operation is outside this eight-tool Pattern capability. Explain the boundary instead of simulating it through UI actions.

If the failure does not establish that the session ended, preserve the exact token and either continue safely or end it explicitly. If session state is uncertain, prefer a fresh read; claim cleanup only from the returned ending result and claim application only from `status: "applied"`.

Completion criterion: the next action is justified by the typed code, mutation replay is avoided whenever outcome is uncertain, and any still-live retained session is explicitly ended.
