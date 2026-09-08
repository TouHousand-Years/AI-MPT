# 31: MCP Sidecar process (stdio translator)

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The stateless, client-supervised MCP Sidecar process from issue 06/24: launched by the MCP client over standard stdio transport, connecting to the app endpoint from issue 30, translating MCP `tools/call` for the five Pattern-mode tools into protocol-neutral app envelopes, and mapping typed app results and failures back to MCP structured content without inventing document-selection or music semantics. Explicit app and document attachment is required before any tool call; the Sidecar owns no project state and performs no model access itself.

**Blocked by:** 30 (App-side IPC broker with owning-thread dispatch)

**Status:** resolved

## Acceptance criteria (demo to owner)

- [x] A real MCP client (the owner's own MCP-capable client or an MCP inspector) launches the Sidecar over stdio, lists the five Pattern-mode tools, and completes a full `get_pattern_context` round trip returning Score Context from the running OpenMPT instance with the issue-28 fixture open.
- [x] Without explicit attachment the client receives `notAttached`; a wrong or closed document ID yields the typed failure, never a guess at the foreground or sole document.
- [x] Sidecar translation is covered by tests at its stdio boundary against a scripted app-endpoint double: envelope translation for all five tools plus `notAttached` / `documentGone` / `owningThreadRequired` / `instanceGone` mapping, per the issue-24 guided cases.
- [x] Sidecar restart is harmless: stateless relaunch plus explicit reattach restores operation with no residue in the app.

## Work log

### 2026-09-08

- The existing stdio translator maps the five Pattern-mode tools
  (`get_pattern_context`, `replace_pattern_segment`, `handoff_for_review`,
  `abort_session`, `release_occupancy`) and preserves typed app results and
  failures unchanged in `structuredContent`.
- The official MCP Inspector CLI was used as the real MCP client. It launched
  `python sidecar/openmpt_mcp.py` over stdio and `tools/list` returned exactly
  the five tools. Launched without explicit `--pipe`/`--instance`/`--document`,
  a call returned `notAttached`. A real `get_pattern_context` against the
  running OpenMPT with `test-fixtures/ai-collab-fixture.mptm` pattern 0
  returned `ok` / `isError: false` with Score Context rows=128, channels=4,
  instruments E-Piano / Warm Pad / Round Bass.
- TDD AC4 RED: the first Sidecar process obtained `occupy=true` and then
  exited; a fresh, explicitly attached Sidecar received a capability/busy
  failure because the app retained the prior occupancy.
- Fix in `AIService.cpp`: the broker assigns monotonic connection ids and
  queues identity-only disconnect notices when an attached connection drops;
  the UI/owning thread drains disconnect notices before new requests and
  releases unhanded state only for the matching connection, so a stale notice
  cannot affect a newer connection and a handed-off human proposal survives.
- A native Sidecar restart test was added, and the integration harness
  startup-failure cleanup now kills and waits for the child process.
- Verification: `.\build-local.ps1` PASS; native suite 3 tests OK; sidecar
  discover suite 25 OK with 3 opt-in tests skipped in the non-native run;
  official Inspector real call PASS.
