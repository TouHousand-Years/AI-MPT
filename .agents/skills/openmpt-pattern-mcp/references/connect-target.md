# Connect the target document

Use this branch when a tool returns `notAttached`, when the requested document is ambiguous, or before switching documents.

## Human handoff

Ask the user to do the following in OpenMPT:

1. Open the intended document and its Patterns tab.
2. Open the **AI / MCP** panel.
3. Click **Connect active doc to Codex**.

Then retry a read. The button publishes an exact application lifetime, document lifetime, pipe, and generation. Window focus alone never changes the target; the most recently published document is the target across running OpenMPT instances.

To work on a different Pattern of the same document, do not republish: switch the binding with [`switch_pattern`](switch-pattern.md).

Treat document selection as a human UI action. Do not infer it from the foreground window or attempt UI automation.

## Switching while a session exists

A newly published target does not displace retained work. Finish the old session with `handoff_for_review` or `abort_session`; use `release_occupancy` only if it remained read-only. The next call can then attach to the newly published target.

Completion criterion: a fresh `get_pattern_context` result has `ok: true` and identifies the intended Pattern in `context.pattern`; a binding is changed only through `switch_pattern`.
