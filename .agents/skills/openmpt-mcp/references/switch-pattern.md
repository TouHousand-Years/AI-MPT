# `switch_pattern`

Request a change of the bound Pattern, or authenticate one while a session is retained. The capability binds one Pattern at a time.

## Pattern switching

The capability binds exactly one Pattern at a time. Read the Sequence order first, then choose an existing zero-based Pattern index or a missing index within the reported format limit. Manual navigation in OpenMPT changes only the human's view; it never changes the Agent binding.

To edit another Pattern in the same document, call `switch_pattern`. With no retained session, an approved request establishes a new session. With a retained session, pass its exact token; approval captures the target from scratch and returns a fresh token, so discard the old token. If the requested format-valid Pattern index does not exist, approval creates it with the source Pattern's row count (or the format-clamped 64-row default when no source exists) and appends it at the effective end of the current Order, before trailing stop markers. Read the returned `status` (`switched`, `created`, `unchanged`, or a typed rejection) before continuing.

Each switch grants the target Pattern's full rows and channels, but proposals remain single-Pattern. Finish the current candidate with `handoff_for_review` or `abort_session` before switching again. For multi-Pattern work, complete and verify one Pattern, end its session, switch, then acquire and verify the next Pattern independently.

## Arguments

```json
{
  "pattern": 2,
  "session": "opaque-token"
}
```

- `pattern` (required): zero-based target Pattern index within the module format's Pattern limit. Approval creates it when it is missing and the current Sequence has room for one more Order entry.
- `session` (optional): the current retained token. **Mandatory while a session is occupied**: a session can only rebind itself with its own token. Omit it only when no session exists, which starts a session-less request.

## Approval and results

The request waits for the human in OpenMPT, where **Approve Pattern switch** / **Reject Pattern switch** show the source and target Pattern and the whole-Pattern grant. The human may instead have saved **Always allow Pattern switching**, which approves legal requests automatically and also resolves a request that is already waiting. The call itself does not return while approval waits; it completes with the final resolution below. `pending_approval` is an app-internal marker, never a tool result. Do not issue a concurrent retry; a second call while approval waits is refused with `approvalPending`.

- `status: "unchanged"`: the target is the already bound Pattern; the binding is unchanged. With a token, that token is kept, the normal idle retention timeout is refreshed, and fresh `context` is returned.
- `status: "switched"`: approval re-captures the target from scratch—fresh baseline and candidate, every row and channel authorized—and returns a new `session` token; a session-less request establishes a retained session here. Discard any old token. The Patterns page display follows the target and the previous selection is cleared; Sequence/Order content and playback position are untouched.
- `status: "created"`: the missing target was created, appended to Order, captured, and bound. `created: true` and `appended_order` identify the structural change. Rejection creates nothing.
- Failure: a target outside the format limit or a full Sequence is refused (`validationFailure`); a rejected switch (`patternSwitchRejected`) keeps the original binding (a retained session keeps its token and refreshes its idle timeout; a session-less request is released); a forced release, disconnect, or document close ends the wait with `occupancyLost`.

## Boundaries

- Uncommitted candidate edits (`candidateExists`) and a frozen proposal block a switch. The proposal returns `busy` when no session is retained and `occupancyLost` once the old token has ended. Apply, reject, or abort that work first; never submit or discard it implicitly.
- Repeated Order references address the same Pattern as any other occurrence; the result reports the target's `order_references` and relationship. Manual navigation in OpenMPT never changes the binding.
- A switch only changes Order when it must create its missing target, in which case it appends exactly one reference. Use `reorder_pattern_order` for reordering. A switch never changes Sequence or opens a multi-Pattern draft. Each proposal and each Pattern Undo touches exactly one Pattern.

Completion criterion: the returned `status` and any `session` are read, the target context was captured, and the old token is never reused.
