# 31: MCP Sidecar process (stdio translator)

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The stateless, client-supervised MCP Sidecar process from issue 06/24: launched by the MCP client over standard stdio transport, connecting to the app endpoint from issue 30, translating MCP `tools/call` for the five Pattern-mode tools into protocol-neutral app envelopes, and mapping typed app results and failures back to MCP structured content without inventing document-selection or music semantics. Explicit app and document attachment is required before any tool call; the Sidecar owns no project state and performs no model access itself.

**Blocked by:** 30 (App-side IPC broker with owning-thread dispatch)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [ ] A real MCP client (the owner's own MCP-capable client or an MCP inspector) launches the Sidecar over stdio, lists the five Pattern-mode tools, and completes a full `get_pattern_context` round trip returning Score Context from the running OpenMPT instance with the issue-28 fixture open.
- [ ] Without explicit attachment the client receives `notAttached`; a wrong or closed document ID yields the typed failure, never a guess at the foreground or sole document.
- [ ] Sidecar translation is covered by tests at its stdio boundary against a scripted app-endpoint double: envelope translation for all five tools plus `notAttached` / `documentGone` / `owningThreadRequired` / `instanceGone` mapping, per the issue-24 guided cases.
- [ ] Sidecar restart is harmless: stateless relaunch plus explicit reattach restores operation with no residue in the app.
