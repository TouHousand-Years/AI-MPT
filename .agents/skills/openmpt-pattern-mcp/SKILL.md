---
name: openmpt-pattern-mcp
description: Operate the OpenMPT Pattern MCP tools to read the current Sequence order, switch the bound Pattern with approval, inspect Score Context, compose or revise tracker cells in a retained Agent Edit Session, and finish with handoff. Use when an OpenMPT document is connected and the task concerns the Sequence order or reading or editing its bound Tracker Pattern; do not use for UI automation, audio review, project-wide settings, or unsupported non-Pattern operations.
---

# OpenMPT Pattern MCP

Use the connected Pattern capability as a one-Pattern-at-a-time proposal workflow. The tools edit a private candidate; a frozen proposal is applied only after human approval or a saved automatic-accept preference, and every result must be read for its actual `status`.

## Pattern switching

The capability binds exactly one Pattern at a time. Read the Sequence order first and use an existing zero-based Pattern index as the switch target. Manual navigation in OpenMPT changes only the human's view; it never changes the Agent binding.

To edit another Pattern in the same document, call `switch_pattern`. With no retained session, an approved request establishes a new session. With a retained session, pass its exact token; approval captures the target from scratch and returns a fresh token, so discard the old token. Read the returned `status` (`switched`, `unchanged`, or a typed rejection) before continuing.

Each switch grants the target Pattern's full rows and channels, but proposals remain single-Pattern. Finish the current candidate with `handoff_for_review` or `abort_session` before switching again. For multi-Pattern work, complete and verify one Pattern, end its session, switch, then acquire and verify the next Pattern independently.

## Route

Load only the files needed for the current branch:

- If no document is attached or the target may be wrong, read [connect-target.md](references/connect-target.md).
- Before any retained read or edit, read [session-lifecycle.md](references/session-lifecycle.md).
- Before interpreting or constructing cells, read [raw-cell-model.md](references/raw-cell-model.md).
- For a tool call, read exactly that tool's file:
  - [`get_pattern_order`](references/get-pattern-order.md)
  - [`switch_pattern`](references/switch-pattern.md)
  - [`get_pattern_context`](references/get-pattern-context.md)
  - [`replace_pattern_segment`](references/replace-pattern-segment.md)
  - [`handoff_for_review`](references/handoff-for-review.md)
  - [`abort_session`](references/abort-session.md)
  - [`release_occupancy`](references/release-occupancy.md)
- On any `ok: false` result, read [recover-errors.md](references/recover-errors.md) before the next mutation.

## Operating contract

1. Read context before reasoning about the music. The bound Pattern and zero-based coordinates come from OpenMPT, not from UI guesses.
2. The binding holds one Pattern. Manual navigation in OpenMPT never rebinds it; only an approved `switch_pattern` does.
3. For edits, acquire one opaque `session` with `get_pattern_context(occupy=true)`; an approved session-less `switch_pattern` also establishes a retained session on the target. Pass that exact token to every later call. While a session is retained, `switch_pattern` requires that token; approval returns a fresh token for the target.
4. Preserve unsupported raw fields and reconstruct every affected segment deliberately. A sparse omission inside a replacement range means an empty cell, not "leave unchanged."
5. Verify the candidate against the original baseline before finishing.
6. End every retained session exactly once with `handoff_for_review`, `abort_session`, or—only when no edits exist—`release_occupancy`.

Completion means the requested context was returned and read-only occupancy released, or an edited candidate was verified and an ending result proves the session ended: `ok: true` from any ending tool, or `status: pending_review` from `handoff_for_review`—even with `ok: false`, because that status proves the candidate was frozen and occupancy released. An ending call rejected before the freeze (for example `occupancyLost` or `stale`) proves nothing; re-read session state before claiming completion. Read the returned `status`: `applied` means the live document changed, `pending_review` means the proposal still waits for the human. Never promise that an automatic application will succeed.
