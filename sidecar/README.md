# Pattern MCP Sidecar

Status: translator + app-side IPC endpoint implemented (issue 30); MCP client wiring is issue 31.
Label: triage

`openmpt_mcp.py` is the translator portion of issue 31 under issue 26: it maps
MCP tools to the protocol-neutral app envelope over the app's named pipe. The
app side it speaks to is implemented: the running OpenMPT application exposes a
current-user named pipe (`AIService.cpp`), the broker validates and queues
envelopes off the owning thread, and the document owning thread alone rechecks
document liveness and executes capability calls through the issue-29 facade.
There is no discovery and no foreground-document fallback anywhere.

## Run and test

Python 3.10+ with the standard library only, Windows for the named pipe.

```powershell
python -m unittest discover -s sidecar -v
python sidecar/openmpt_mcp.py --help
```

The process tests launch the Sidecar over stdin/stdout and use an independent
scripted Windows named-pipe peer. They cover all five translations, explicit
attachment, error-layer preservation, fragmented replies, rejected attachment,
disconnect without mutation replay, and rejection of remote pipe names.
`test_probe.py` covers the probe client against the same scripted peer. They
do not load or modify a music document.

The opt-in native integration test runs the real application and real pipe:

```powershell
$env:OPENMPT_RUN_NATIVE_INTEGRATION = '1'
python -m unittest sidecar.test_native_integration -v
```

It drives all five tools through the running app, walks the guided failure
cases (`notAttached`, `documentGone`, `owningThreadRequired`, `instanceGone`),
and asserts from the integration trace that no AI IPC path executed on the
realtime audio callback while playback and IPC traffic ran concurrently.

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

## MCP client configuration

Configure the MCP client to launch `python` with `sidecar/openmpt_mcp.py`,
`--pipe`, `--instance`, and `--document`. Use an absolute script path and the
exact three values from the application. Launching without all three values is
allowed for listing tools, but calls return `notAttached`. A rejected
attachment or lost connection requires relaunching with explicit identities;
the process never retries a possibly applied call.

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
not interpreted or retargeted. Transport disconnect becomes `instanceGone`;
malformed framing/results become `schemaFailure` and close the connection.

One extension exists for diagnostics: a call envelope with `"direct":true` is
never queued. The broker attempts the model read on its own thread and the
owning-thread guard rejects it with `owningThreadRequired`, making the
issue-24 dispatch seam observable from the transport. The Sidecar never sends
this flag; only `probe.py` does.

The five tool schemas are in `openmpt_mcp.py`. Pattern indices are app-owned;
there is no Pattern selector on this surface. Ranges and channels are zero-based.
Segment entries use absolute row indices with all six raw cell fields; omitted
rows represent empty desired cells. The app alone validates musical semantics,
format constraints, preservation and envelope approvals.

An expansion request may remain blocked on the pipe while the app's non-modal
UI waits for its human decision. The broker remains able to service human
force-release, app shutdown, and document closure during that wait. The
Sidecar does not invent a separate occupancy timer, lease, or project state.
