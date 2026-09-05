# 30: App-side IPC broker with owning-thread dispatch

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The app-side IPC endpoint from issue 06/24, reduced to the first slice: a current-user Windows named pipe (ACL-scoped, no TCP listener) carrying length-prefixed UTF-8 protocol-neutral envelopes between the running app and one Sidecar. The broker performs framing, validation, and limit checks away from the owning thread and only queues; the document owning thread alone rechecks document liveness, captures the immutable Pattern snapshot, and executes capability calls through the issue-29 facade. Explicit app and document attachment is mandatory — no guessing the foreground or sole document. First-slice error taxonomy: `notAttached`, `documentGone`, `owningThreadRequired`, `instanceGone`, plus connection/schema failure and a safe `internalError`. Multi-instance discovery, leases, receipts, events, and fault-injection hardening remain deferred per issue 26.

**Blocked by:** 29 (Pattern capability seam with the five Pattern-mode tools)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [ ] Starting the app exposes the IPC endpoint; a small command-line probe client can attach to the app and an explicitly addressed open document, then call `get_pattern_context` over the pipe and receive the real Score Context produced on the document owning thread.
- [ ] Guided failure cases are demonstrable from the real transport: request without attach → `notAttached`; document closed while queued → `documentGone` (old ID never drifts to another document); broker attempting a direct model read → rejected `owningThreadRequired`; app exit → `instanceGone`.
- [ ] Typed failures identify their layer and stable code and never change attachment or retarget a request; no IPC read/write/wait/query path executes on a realtime audio callback (verified by test or instrumentation).
- [ ] Owner can run the probe client themselves: each command above produces a visible, understandable result against the running app.
