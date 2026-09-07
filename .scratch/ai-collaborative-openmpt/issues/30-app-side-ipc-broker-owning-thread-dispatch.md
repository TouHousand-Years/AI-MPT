# 30: App-side IPC broker with owning-thread dispatch

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The app-side IPC endpoint from issue 06/24, reduced to the first slice: a current-user Windows named pipe (ACL-scoped, no TCP listener) carrying length-prefixed UTF-8 protocol-neutral envelopes between the running app and one Sidecar. The broker performs framing, validation, and limit checks away from the owning thread and only queues; the document owning thread alone rechecks document liveness, captures the immutable Pattern snapshot, and executes capability calls through the issue-29 facade. Explicit app and document attachment is mandatory — no guessing the foreground or sole document. First-slice error taxonomy: `notAttached`, `documentGone`, `owningThreadRequired`, `instanceGone`, plus connection/schema failure and a safe `internalError`. Multi-instance discovery, leases, receipts, events, and fault-injection hardening remain deferred per issue 26.

**Blocked by:** 29 (Pattern capability seam with the five Pattern-mode tools)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [x] Starting the app exposes the IPC endpoint; a small command-line probe client can attach to the app and an explicitly addressed open document, then call `get_pattern_context` over the pipe and receive the real Score Context produced on the document owning thread.
- [x] Guided failure cases are demonstrable from the real transport: request without attach → `notAttached`; document closed while queued → `documentGone` (old ID never drifts to another document); broker attempting a direct model read → rejected `owningThreadRequired`; app exit → `instanceGone`.
- [x] Typed failures identify their layer and stable code and never change attachment or retarget a request; no IPC read/write/wait/query path executes on a realtime audio callback (verified by test or instrumentation).
- [x] Owner can run the probe client themselves: each command above produces a visible, understandable result against the running app.

## Work log — 2026-09-07

The issue-29 work had already landed the app-side broker in `AIService.cpp`
(current-user ACL'd named pipe, 4-byte length + UTF-8 JSON framing, envelope
validation and 4 MiB limit checks on a worker thread, validate-and-queue-only,
UI/owning-thread dispatch through the `PatternCapability` facade). This ticket
completed the issue-30 slice on top of it:

- **Probe client (`sidecar/probe.py`)**: owner-facing CLI with `attach`,
  `context` (prints the real Score Context), six guided failure cases
  (`not-attached`, `stale-document`, `wrong-instance`, `owning-thread`,
  `document-gone`, `instance-gone`), and a `demo` walk that performs a real
  read then the whole taxonomy in gentlest-first order, printing typed results
  and PASS/FAIL lines. The two destructive steps (close document, exit app)
  are guided human steps; the probe pauses and says when.
- **`owningThreadRequired` from the transport**: a call envelope with
  `direct:true` is never queued — the broker thread attempts the model read
  and the owning-thread guard rejects it, making the issue-24 seam observable.
- **Realtime-audio instrumentation (AC3)**: every AI IPC read/write/wait
  registers its thread for its duration (`IpcThreadScope`); the audio callback
  checks the registry every buffer via `AI::AudioCallbackIpcCheck()` and
  traces a VIOLATION line on conflict. The integration harness plays the
  fixture while IPC traffic flows and asserts the trace is violation-free and
  that the check itself ran on the audio thread.
- **Integration harness hardening**: `IntegrationHost` now starts playback
  (audio callback exercised), forces the Patterns view to activate despite the
  hidden app window, disables follow-song for the binding view, and publishes
  the endpoint only after the requested pattern has stayed bound for two
  consecutive ticks (page init completes asynchronously and resets once).
- **Panel**: the AI/MCP panel now lists every open document with its lifetime
  ID (explicit attach needs it) and refreshes on document changes.
- **Tests**: `sidecar/test_probe.py` (12 cases against the scripted endpoint
  double) and an opt-in native integration test
  `test_guided_failure_cases_from_real_transport` covering all typed failures
  plus app-exit → `instanceGone` from the real pipe.

Verification:

- `python -m unittest discover -s sidecar` — 24 passed (2 opt-in/skipped).
- `OPENMPT_RUN_NATIVE_INTEGRATION=1 python -m unittest sidecar.test_native_integration` — all OK: five-tool round trip on both fixture patterns through the real sidecar and pipe, guided failure cases, app-exit `instanceGone`, and the no-VIOLATION/audio-callback-instrumented trace assertions.
- `./build-local.ps1 -Test` — PASS with the two-voice demo report.
- Manual owner-demo check: `probe.py context` returned the real fixture Score
  Context (instruments, samples, timing, limits, cells) captured on the owning
  thread; `case owning-thread` and `case not-attached` printed their expected
  typed failures with exit-code signalling.

Design interpretations:

- A `call` on an attached connection that names a different document is
  refused at the broker attachment layer as `notAttached` (the connection is
  pinned to one explicit attachment). `documentGone` at the owning thread is
  therefore demonstrated via `attach` to an absent document ID (automated
  test) and via the guided human step of closing the attached document
  (`case document-gone`, optional `--drift-check` re-send showing the stale ID
  never drifts to a new document). Liveness is always re-checked by identity
  at dispatch time on the owning thread.
- `instanceGone` on app exit is realized on the client side: the pipe
  disappears with the process, and lost connections map to `instanceGone`
  (never a replay).
- The "closed while queued" timing window is not externally distinguishable
  from a post-close request at 100 ms dispatch granularity; the dispatch-time
  liveness recheck is the mechanism under demonstration either way.
- Integration playback must not steer the view: follow-song is disabled for
  the binding view, and the endpoint is published only after the bound
  pattern settles, because pattern-page initialization completes
  asynchronously and resets the current pattern once.

Code review was intentionally not started, per the owner's requested pause
point.
