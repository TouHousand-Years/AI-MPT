# `switch_pattern`

Request a change of the bound Pattern, or authenticate one while a session is retained. The capability binds one Pattern at a time.

## Arguments

```json
{
  "pattern": 2,
  "session": "opaque-token"
}
```

- `pattern` (required): zero-based target Pattern index. It must be an existing valid Pattern; this tool never creates one.
- `session` (optional): the current retained token. **Mandatory while a session is occupied**: a session can only rebind itself with its own token. Omit it only when no session exists, which starts a session-less request.

## Approval and results

The request waits for the human in OpenMPT, where **Approve Pattern switch** / **Reject Pattern switch** show the source and target Pattern and the whole-Pattern grant. The human may instead have saved **Always allow Pattern switching**, which approves legal requests automatically and also resolves a request that is already waiting. The call itself does not return while approval waits; it completes with the final resolution below. `pending_approval` is an app-internal marker, never a tool result. Do not issue a concurrent retry; a second call while approval waits is refused with `approvalPending`.

- `status: "unchanged"`: the target is the already bound Pattern; the binding is unchanged. With a token, that token is kept, the normal idle retention timeout is refreshed, and fresh `context` is returned.
- `status: "switched"`: approval re-captures the target from scratch—fresh baseline and candidate, every row and channel authorized—and returns a new `session` token; a session-less request establishes a retained session here. Discard any old token. The Patterns page display follows the target and the previous selection is cleared; Sequence/Order content and playback position are untouched.
- Failure: an invalid or non-existent target is refused (`validationFailure`); a rejected switch (`patternSwitchRejected`) keeps the original binding (a retained session keeps its token and refreshes its idle timeout; a session-less request is released); a forced release, disconnect, or document close ends the wait with `occupancyLost`.

## Boundaries

- Uncommitted candidate edits (`candidateExists`) and a frozen proposal block a switch. The proposal returns `busy` when no session is retained and `occupancyLost` once the old token has ended. Apply, reject, or abort that work first; never submit or discard it implicitly.
- Repeated Order references address the same Pattern as any other occurrence; the result reports the target's `order_references` and relationship. Manual navigation in OpenMPT never changes the binding.
- A switch never creates a Pattern, reorders Orders, changes Sequence, or opens a multi-Pattern draft. Each proposal and each Undo touches exactly one Pattern.

Completion criterion: the returned `status` and any `session` are read, the target context was captured, and the old token is never reused.
