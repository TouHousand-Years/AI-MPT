# Pattern MCP Sidecar

Status: implemented against a scripted endpoint; native integration pending.
Label: triage

This is the translator portion of issue 31 under issue 26. The app-side
capability facade, named-pipe broker, document identities, occupancy UI and
proposal UI are not implemented yet. This directory does **not** make the
existing OpenMPT executable MCP-capable. Do not start the owner acceptance
checklist on the basis of these tests.

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
They do not load or modify a music document.

Once issue 30 supplies real identities, configure the MCP client to launch
`python` with `sidecar/openmpt_mcp.py`, `--pipe`, `--instance`, and `--document`.
Use an absolute script path and the exact three values from the application.
There is no foreground-document fallback, discovery, or attach tool. Launching
without all three values is allowed for listing tools, but calls return
`notAttached`. A rejected attachment or lost connection requires relaunching
with explicit identities; the process never retries a possibly applied call.

The supported MCP revision is 2025-11-25. It uses newline-delimited UTF-8
JSON-RPC and returns both structured tool results and a text representation,
following the [stdio transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports)
and [tool result](https://modelcontextprotocol.io/specification/2025-06-18/server/tools)
specifications. Standard output contains protocol messages only.

## App envelope proposed for issue 30

Each direction uses a four-byte unsigned little-endian byte length followed by
UTF-8 JSON, maximum 4 MiB per frame. The local pipe must be created by the app
with a current-user ACL and remote connections rejected. The Sidecar rejects
remote names; it cannot install or enforce the server's pipe ACL.

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

The five tool schemas are in `openmpt_mcp.py`. Pattern indices are app-owned;
there is no Pattern selector on this surface. Ranges and channels are zero-based.
Segment entries use absolute row indices with all six raw cell fields; omitted
rows represent empty desired cells. The app alone validates musical semantics,
format constraints, preservation and envelope approvals. This schema must be
implemented and checked on the owning-thread facade before live integration.

An expansion request may remain blocked on the pipe while the app's non-modal
UI waits for its human decision. The broker must remain able to service human
force-release, app shutdown, and document closure during that wait. The Sidecar
does not invent a separate occupancy timer, lease, or project state.
