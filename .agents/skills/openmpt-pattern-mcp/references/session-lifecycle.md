# Agent Edit Session lifecycle

## Choose the path

- One-shot inspection: call `get_pattern_context` without `occupy`; no session token is retained.
- Multi-read analysis or any edit: call `get_pattern_context` with `occupy: true`, retain the returned opaque `session`, and finish it explicitly. An approved session-less `switch_pattern` also establishes a retained session on the target.

The session binds the current Pattern, its captured baseline, a private candidate, format/resource dependencies, and an initial edit envelope. The default retention timeout is five minutes; successful retained reads and writes refresh it. Never invent, transform, cache for later, or reuse a lost token.

To move the same session to another Pattern, call `switch_pattern` with that token (see [switch-pattern.md](switch-pattern.md)). Approval re-captures the target in full and returns a fresh token; the old token is dead. Manual navigation in OpenMPT never rebinds a session.

## Edit loop

1. Acquire the full candidate context with `occupy: true` unless a bounded range is sufficient.
2. Decide changes from `context.cells`, timing, format limits, and available instruments or samples.
3. Apply bounded channel segments with `replace_pattern_segment`.
4. Read candidate ranges using the same `session` and compare with `baseline: true` reads where needed.
5. If the verified diff fulfills the request, call `handoff_for_review`. If it does not, continue editing or call `abort_session`.
6. To work on another Pattern first, ensure no candidate edits exist, switch with the retained token, and continue with the fresh token.

Only one connection owns a retained editing session. A human may keep reading and auditioning the document, but document dependency changes make the session stale.

## Mandatory ending

- Edited and ready: `handoff_for_review`.
- Edited but unwanted, unsafe, or impossible to finish: `abort_session`.
- Read-only retained work: `release_occupancy`.

Completion criterion: the ending result proves the retained session ended. Accept `ok: true` from `handoff_for_review`, `abort_session`, or `release_occupancy`; for `handoff_for_review`, `ok: false` with `status: pending_review` also proves the candidate was frozen and occupancy released. An ending call rejected before the freeze (for example `occupancyLost` or `stale`) does not; re-read session state before claiming completion. After `handoff_for_review`, report `applied` only for that status, and `pending_review` for a proposal that still waits for the human—including a failed automatic attempt (`ok: false` with `status: pending_review`).
