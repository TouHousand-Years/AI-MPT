# Pattern MCP Sidecar

Status: implemented (issues 30 and 31, plus the cross-Pattern order and switch slice); verified with the official MCP Inspector CLI.
Label: triage

`openmpt_mcp.py` implements issue 31 under issue 26: it maps
MCP tools to the protocol-neutral app envelope over the app's named pipe. The
app side it speaks to is implemented: the running OpenMPT application exposes a
current-user named pipe (`AIService.cpp`), the broker validates and queues
envelopes off the owning thread, and the document owning thread alone rechecks
document liveness and executes capability calls through the issue-29 facade.
There is no foreground-document fallback. For normal Codex use, the owner can
explicitly publish the active document from OpenMPT's AI / MCP panel; the
long-running Sidecar reads that current-user target file and attaches to the
exact published lifetime identities. The translator also exposes the
cross-Pattern slice: `get_pattern_order` reads the current Sequence order
through the app without requesting occupancy, and `switch_pattern` requests or
performs an approved rebinding to another Pattern while the connection keeps
one retained session.

## Run and test

Python 3.10+ with the standard library only, Windows for the named pipe.

```powershell
python -m unittest discover -s sidecar -v
python sidecar/openmpt_mcp.py --help
```

The process tests launch the Sidecar over stdin/stdout and use an independent
scripted Windows named-pipe peer. They cover all seven translations, explicit
and published-target attachment, guarded target switching, error-layer
preservation, fragmented replies, rejected attachment, disconnect without
mutation replay, and rejection of remote pipe names.
`test_probe.py` covers the probe client against the same scripted peer. They
do not load or modify a music document.

The opt-in native integration test runs the real application and real pipe:

```powershell
$env:OPENMPT_RUN_NATIVE_INTEGRATION = '1'
python -m unittest sidecar.test_native_integration -v
```

It drives all seven tools through the running app, walks the guided failure
cases (`notAttached`, `documentGone`, `owningThreadRequired`, `instanceGone`),
and asserts from the integration trace that no AI IPC path executed on the
realtime audio callback while playback and IPC traffic ran concurrently.

## Official MCP Inspector CLI examples

The official [MCP Inspector](https://github.com/modelcontextprotocol/inspector)
CLI is a real MCP client and was used to verify this Sidecar end to end:

```powershell
npx --yes @modelcontextprotocol/inspector --cli python C:\<abs>\sidecar\openmpt_mcp.py --pipe <pipe> --instance <instance> --document <document> -- --method tools/list --format json

npx --yes @modelcontextprotocol/inspector --cli python C:\<abs>\sidecar\openmpt_mcp.py --pipe <pipe> --instance <instance> --document <document> -- --method tools/call --tool-name get_pattern_context --tool-arg occupy=true --format json
```

The Sidecar target flags (`--pipe`, `--instance`, `--document`) appear before
the bare `--` and are given to the launched Sidecar process; the flags after
the bare `--` are Inspector's own flags (`--method`, `--tool-name`,
`--tool-arg`, `--format`), selecting the MCP operation to run. In the launch
command, `--cli` selects Inspector's CLI mode and `python` plus the absolute
script path launch the Sidecar as the MCP server. `tools/list` returns exactly
the seven Pattern-mode tools; `tools/call get_pattern_context` against the
running app with the issue-28 fixture open returned the real Score Context in
`structuredContent`.

## Probe client for the owner demo (issue 30)

`probe.py` is a small command-line client for the app endpoint. It attaches to
one explicitly addressed app instance and document, reads the real Score
Context, and walks the guided failure taxonomy. Closing a document or exiting
the app is a human step; the probe pauses and says when.

Owner workflow:

1. Start the app, open the MPTM fixture, and open its Patterns tab.
2. Open the "AI / MCP" panel. It lists the pipe name, the instance ID, and
   every open document with its lifetime ID. With MCP enabled (default) the
   pipe is live at startup and the panel shows "MCP ready".
3. Run, with the exact values from the panel:

```powershell
python sidecar/probe.py demo --pipe "\\.\pipe\OpenMPT-AI-<pid>-{guid}" --instance "{guid}" --document "document-1" --drift-check
```

`demo` performs a real `get_pattern_context` read, then each guided case in
order: `not-attached`, `wrong-instance`, `stale-document`, `owning-thread`,
then the two guided destructive steps (`document-gone`, `instance-gone`),
printing the typed result and a PASS/FAIL line for each. Individual steps are
available as `attach`, `context`, and `case <name>`; each case exits non-zero
when its expected typed failure is not observed.

## Recommended Codex setup: configure once, select documents in OpenMPT

Add one stable STDIO server to the user-level `~/.codex/config.toml`, or to a
trusted project's `.codex/config.toml`. Use forward slashes in the Windows path:

```toml
[mcp_servers.openmpt]
command = "python"
args = ["C:/absolute/path/to/OpenMPT-for-AI/sidecar/openmpt_mcp.py", "--auto-target"]
required = false
startup_timeout_sec = 10
tool_timeout_sec = 300
```

The equivalent one-time CLI command is:

```powershell
codex mcp add openmpt -- python C:/absolute/path/to/OpenMPT-for-AI/sidecar/openmpt_mcp.py --auto-target
```

Restart Codex once after adding this entry. After that, changing OpenMPT
documents does not require editing or restarting Codex:

1. Open the document and its Patterns tab in OpenMPT.
2. Open the **AI / MCP** panel.
3. Click **Connect active doc to Codex**.
4. Use the seven Pattern tools from the existing Codex task.

The button atomically publishes the exact pipe, application-lifetime ID,
document-lifetime ID, and a fresh publication generation to
`%LOCALAPPDATA%\OpenMPT\AI\codex-target.json`. `--auto-target` reads that
standard current-user path. `--target-file <path>` is available for a custom
stable location.

Publishing is rejected while OpenMPT has retained occupancy or a proposal
awaiting review. As a second guard, the Sidecar will not switch a connection
when it knows retained work is active; only `handoff_for_review`,
`abort_session`, or `release_occupancy` continues against the old explicit
identity. The newly published target is picked up by the next call after the
old work ends. Re-publishing the same document creates a new generation, which
is an explicit request to clear a latched attachment error and reconnect.

A successful `switch_pattern` that returns a `session` token—`status: switched`,
or `status: unchanged` carrying the retained token—counts as retained work, so
the Sidecar pins the published document until the retained session ends. A
session-less `status: unchanged` leaves retention unchanged. A
`handoff_for_review` that returns `status: pending_review` ends that retention
even when `ok` is false, because occupancy has ended on the app side.

The target file is not automatic foreground tracking: opening or focusing a
different document never changes it. Multiple OpenMPT instances share the
same current-user publication location, so the most recently clicked button
selects the target.

## Explicit MCP client configuration for diagnostics

Configure the MCP client to launch `python` with `sidecar/openmpt_mcp.py`,
`--pipe`, `--instance`, and `--document`. Use an absolute script path and the
exact three values from the application. Launching without all three values is
allowed for listing tools, but calls return `notAttached`. This legacy mode is
useful for the probe, Inspector, and integration tests. It cannot be combined
with `--auto-target` or `--target-file`. A rejected attachment or lost
connection remains latched, and the process never retries a possibly applied
call; in published-target mode, click the OpenMPT connection button again to
explicitly create a new generation before reconnecting.

The supported MCP revision is 2025-11-25. It uses newline-delimited UTF-8
JSON-RPC and returns both structured tool results and a text representation,
following the [stdio transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports)
and [tool result](https://modelcontextprotocol.io/specification/2025-06-18/server/tools)
specifications. Standard output contains protocol messages only.

## App envelope (issue 30)

Each direction uses a four-byte unsigned little-endian byte length followed by
UTF-8 JSON, maximum 4 MiB per frame. The app creates the pipe with a
current-user ACL and rejects remote clients. The Sidecar rejects remote names;
it cannot install or enforce the server's pipe ACL.

The first request on a connection is:

```json
{"version":1,"operation":"attach","instance":"APP-LIFETIME-ID","document":"DOCUMENT-LIFETIME-ID"}
```

Only after an `{"ok":true}` response does it send tool envelopes:

```json
{"version":1,"operation":"call","instance":"APP-LIFETIME-ID","document":"DOCUMENT-LIFETIME-ID","tool":"get_pattern_context","arguments":{"occupy":true}}
```

The app returns an object with boolean `ok`, and on failure an `error` object
with `layer`, `code`, and diagnostic fields. The Sidecar carries the result
unchanged in `structuredContent`. App errors such as `documentGone`,
`owningThreadRequired`, occupancy loss, stale and cell validation failures are
not rewritten or retargeted; the `status` field is used only to track the
retained-session lifecycle. Transport disconnect becomes `instanceGone`;
malformed framing/results become `schemaFailure` and close the connection.

The app pipe accepts multiple simultaneous local connections. Their envelopes
are queued and dispatched deterministically on the UI thread, so attached
clients can perform one-shot read-only calls without competing for the pipe.
An `occupy=true` read claims the single retained session for that connection;
until it releases or hands off the session, calls from other connections receive
`busy`. A successful `switch_pattern` keeps that same retained session, now
bound to the target Pattern with a fresh token. `get_pattern_order` is always
available: it is a session-less read that never claims or leaks occupancy.
Attaching or disconnecting a read-only client never steals or releases
another connection's retained state.

One extension exists for diagnostics: a call envelope with `"direct":true` is
never queued. The broker attempts the model read on its own thread and the
owning-thread guard rejects it with `owningThreadRequired`, making the
issue-24 dispatch seam observable from the transport. The Sidecar never sends
this flag; only `probe.py` does.

The seven tool schemas are in `openmpt_mcp.py`. Pattern indices are app-owned
and zero-based; `switch_pattern` is the only way to change the bound Pattern,
and manual navigation in OpenMPT never rebinds it. Ranges and channels are also
zero-based.
Segment entries use absolute row indices with all six raw cell fields; omitted
rows represent empty desired cells. The app alone validates musical semantics,
format constraints, preservation and envelope approvals.

An expansion or switch request may remain blocked on the pipe while the app's
non-modal UI waits for its human decision. The app-internal `pending_approval`
marker holds the queued request; the client call completes only when the
decision arrives, with the final tool result (an approved mutation or a typed
rejection). The marker itself is never returned to the MCP client. The broker
remains able to service human force-release, app shutdown, and document closure
during that wait. The Sidecar does not invent a separate occupancy timer,
lease, or project state.
