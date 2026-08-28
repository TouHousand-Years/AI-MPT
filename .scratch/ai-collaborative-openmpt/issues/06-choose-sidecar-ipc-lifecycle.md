# Choose MCP sidecar IPC and lifecycle

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by:

## Question

How should the local MCP sidecar discover and connect to running project instances, invoke the shared Application Capability layer, observe revisions and events, handle concurrency and cancellation, and fail without endangering the OpenMPT UI or audio engine?

## Comments

The design was grilled in rounds and confirmed by the human collaborator on 2026-08-28. It refines the thread, revision, transaction, event, and Agent Edit Session invariants established by issue 05 without binding the Application Capability layer itself to MCP. Issue 08 later narrows the first-release proposal workflow so candidate operations remain private until one human-approved accepted subset is applied atomically.

## Answer

### Process topology and trust boundary

Each MCP client launches and supervises its own stateless local sidecar over MCP's standard stdio transport. Every running OpenMPT application process exposes an app-side IPC endpoint; the sidecar translates MCP resources and tools into a protocol-neutral Application Capability envelope. No MCP parsing or lifecycle management belongs in the UI, document, or audio core.

The first release is Windows-only, local-machine-only, and scoped to one signed-in user session. App-side IPC uses a Windows named pipe protected by an ACL for the current user and never opens a TCP listener. A same-user process may discover projects, read capabilities, and request an Agent Edit Session. Revision checks, the exclusive write lease, visible UI state, and authoritative human cancellation protect project mutation. Remote authentication, cross-host discovery, and non-Windows transports are out of scope even though the capability envelope remains transport-neutral.

### Discovery, identity, and handshake

Each app process atomically publishes a small registration record in a current-user-only runtime directory. Its filename includes the app instance ID rather than relying on a PID; its contents include the pipe name, PID, process start time, app instance ID, and a random nonce. The app removes its own record on normal exit. A later app startup may remove an abandoned record only after proving that the recorded process identity is no longer live. A sidecar ignores stale records and never deletes another live process's record.

The sidecar treats a record only as a connection hint. It connects to the pipe and verifies the PID, process start time, instance ID, nonce challenge, and current-user boundary during the handshake. The peers also exchange:

- protocol major and minor versions;
- app build and app instance identity;
- capability and schema manifests;
- maximum message, concurrency, time, and resource limits.

An incompatible protocol or capability-schema major version is rejected. Minor-version peers may use the intersection of their advertised capabilities. Envelopes may ignore unknown optional fields, but an unknown required field, operation, enumeration value, or schema major is an error. Every response reports the schema version actually used; no semantic downgrade is silent.

An app instance reports all open documents. A document is addressed by `(appInstanceId, documentId)`, stable only for that app/document lifetime, and is described with its title, canonical path when saved, format, and current revision. Unsaved documents receive an ephemeral ID. When more than one target matches, the client must explicitly attach rather than guessing from the active or foreground window. App restart creates new app and document IDs: a sidecar may rediscover the app but may not automatically reattach even when a path looks identical.

### App-side protocol and dispatch

The named pipe carries length-prefixed UTF-8 JSON envelopes, not MCP messages. A request includes a request ID, operation, attached document ID, negotiated schema version, absolute deadline, payload, and, where applicable, `expectedRevision`, session identity, session sequence, and operation ID. Large binary resources are never embedded in JSON.

The IPC broker performs framing, authentication, envelope validation, and limit checks away from the owning thread. Both queries and mutations are then dispatched to the document owning thread. Expensive pure work may run on workers only from immutable, revision-bound snapshots; the owning thread captures the snapshot and performs final validation and commit. No IPC read, write, wait, query, mutation, or render path may execute on a realtime audio callback.

Every document-mutation request contains one complete command batch. The app validates the whole batch against the current module format and `expectedRevision`, then either commits it completely or leaves state unchanged. One successful batch creates one undo step, increments the document revision once, records one operation receipt, and emits one capability event. During proposal construction, candidate operations instead target an explicitly identified private proposal state and create no document revision, undo step, event, or mutation receipt. Issue 08 permits only the final human-approved accepted subset to become one document-mutation batch.

Requests are rejected before reaching the owning thread when their framing, size, schema, or advertised complexity exceeds a negotiated limit. Work that cannot meet the app's bounded commit budget is rejected with `resourceLimitExceeded`; an atomic batch is not split into partially visible commits to evade that budget.

### Revisions, events, and uncertain outcomes

Each document maintains a monotonically increasing `revision` and `eventSequence`. Query results identify the revision from which their immutable snapshot was captured. A successful atomic batch emits one summary event after commit. Events are invalidation and synchronization signals, not a permanent journal from which the entire project must be reconstructed.

The app retains a bounded ring of events. A reconnecting subscriber supplies its last sequence: the app replays the missing suffix when available, otherwise it returns `resyncRequired` and the client obtains a new revision-bound snapshot. Every connection has bounded response and event queues; responses take priority. When an event queue fills, pending events collapse into one `resyncRequired`. A client that continues not to read is disconnected. IPC backpressure never blocks the owning thread or audio engine.

Every mutation carries a client-generated globally unique `operationId`. Before returning success, the app records a lightweight operation receipt containing the payload hash, result, committed revision, undo/provenance ID, and completion time. During the document lifetime:

- the same ID and payload return the existing receipt without re-executing;
- the same ID with a different payload returns `operationIdConflict`;
- a reattached client may call the read-only receipt query to determine an uncertain result;
- a missing or evicted receipt returns `outcomeUnknown`, never an automatic retry.

Receipts contain hashes and results rather than full payloads. The app retains them until document close subject to a default limit of 10,000 per document, evicting the oldest receipts from completed sessions first and tracking the eviction watermark. Receipts are not written into module files and do not survive an app restart.

After commit and receipt recording, the originating connection queues the mutation response before its corresponding event. Other subscribers need only observe event-sequence order. A client uses the response or receipt, never packet arrival order alone, as the authority for mutation outcome.

### Agent Edit Session and concurrency

Attaching and reading do not acquire a write lease. Immediately before its first candidate mutation or document apply, a client explicitly begins an Agent Edit Session for one document. The session owns the single project-wide write lease and is bound to the client connection, document, and session ID. Other clients may continue revision-bound reads of committed state but may neither write nor queue for the lease; a competing request immediately receives `projectBusy` with non-sensitive session timing information.

The lease is renewed by a default five-second heartbeat. A definitive pipe disconnect ends it immediately; otherwise 30 seconds without a valid heartbeat marks it orphaned. A separately configurable total session limit defaults to ten minutes. Expiry cancels uncommitted work and releases the lease; it never commits automatically. A disconnected write session cannot be resumed. The client must reconnect, reattach, read the latest revision, and begin a new session.

Each session mutation supplies a strictly increasing `sessionSequence` and `expectedRevision`. Only one mutation per session is accepted at a time. A gap returns `sequenceGap`; an old sequence is resolved through its operation receipt or reported as a conflict. Queries may be submitted concurrently but only observe immutable committed revisions.

A session ends explicitly in one of three ways:

- `completeSession` finishes the workflow and releases the lease;
- `handoffForReview` releases the lease and returns the unchanged base revision, immutable Change Proposal identity, session provenance, and review artifacts;
- `abortSession` cancels and discards the private candidate state and releases the lease.

Continuing after handoff or applying an accepted subset requires a new session against the then-current revision. The OpenMPT UI displays connected-client count, target document, a short session identifier, current phase, candidate Review Unit count, base and current revisions, lease time remaining, and prominent Cancel and Handoff controls. Users may continue reading, navigating, visually inspecting, and auditioning while a session is active, but project writes, undo, and redo remain frozen until release.

UI Cancel is authoritative. It rejects new work, removes queued work, cooperatively cancels executing mutations and renders, then closes the session and releases the lease. A sidecar cannot delay, override, renew, or reopen a user-cancelled session.

### Cancellation, deadlines, disconnects, and shutdown

Cancellation has explicit phase semantics:

- `queued`: remove the request with no state change;
- `executing`: cancel cooperatively and discard the current uncommitted batch;
- `committing`: finish the bounded atomic commit or its complete rollback, then report that cancellation arrived during commit;
- `completed`: return `alreadyCompleted`.

Every request has an absolute deadline. Expiry while queued prevents execution; expiry while executing follows the same cooperative path as cancellation. A deadline cannot interrupt the commit critical section and instead marks the response late. Request deadlines are independent of the total session lifetime.

On a definitive sidecar disconnect, the app cancels queued, executing, and render work, discards the current private candidate or uncommitted apply batch, and releases the lease. Already completed document mutations and their receipts remain authoritative. If the connection disappears during the commit critical section, the app still completes commit or rollback and records the receipt before cleanup.

When OpenMPT begins shutting down, it rejects new attaches, sessions, and requests with `appShuttingDown`; cancels queued, executing, and render work; completes or rolls back any commit critical section; removes temporary resources and the registration record; and disconnects IPC. Shutdown never waits indefinitely for a sidecar.

### Offline render and resource lifecycle

AI audition is a bounded offline render from either an immutable document snapshot identified by revision and Musical Range or the exact accepted subset of an immutable Change Proposal identified as specified by issue 08; it neither drives nor depends on live playback. The app owns a current-user-only temporary resource directory. A successful render returns an opaque handle plus media type, size, hashes, source identity, range, and expiry. The sidecar reads that handle in bounded chunks through a dedicated resource operation and cannot supply an arbitrary filesystem path.

Release, request cancellation, session timeout, app shutdown, and expiry remove the associated resources. App startup safely sweeps abandoned expired resources. Initial advertised defaults are:

- 4 MiB maximum JSON envelope;
- 16 in-flight requests per connection;
- one offline render per document;
- 60 seconds and 256 MiB maximum output per render;
- 512 MiB temporary resources per session.

The app owns the hard limits and publishes them in the handshake manifest. A sidecar may request tighter limits but cannot widen them.

### Errors, abuse handling, and diagnostics

Failures use stable typed errors, including `invalidRequest`, `unsupportedCapability`, `instanceGone`, `documentGone`, `notAttached`, `revisionConflict`, `projectBusy`, `operationIdConflict`, `sequenceGap`, `validationFailed`, `resourceLimitExceeded`, `deadlineExceeded`, `cancelled`, `alreadyCompleted`, `resyncRequired`, `outcomeUnknown`, `internalError`, and `appShuttingDown`. Each error includes the request ID, a retryability indication, and safe structured details. The sidecar maps these errors to MCP without inferring semantics from human-readable text.

Handshake or identity failure, oversized or malformed framing, invalid UTF-8, and sustained flow-control violations close the connection. Well-framed business errors receive typed responses; repeated minor protocol violations cross a small implementation-defined threshold and then disconnect. Frame limits are checked before allocation, parsing, decompression, or dispatch.

Default diagnostics record connection and negotiated versions, operation name, runtime document ID, revision, session and request identifiers, timing, cancellation phase, result type, and resource use. They do not record payloads, note data, audio, project paths, or authentication nonces. Detailed content logging requires an explicit, temporary, UI-visible user choice.

### Verification requirements

Implementation is not complete until protocol and state-machine tests plus real OpenMPT integration and fault-injection tests cover:

- discovery across multiple apps and documents, ambiguous attach, stale registrations, and PID reuse;
- ACLs, handshake and schema negotiation, frame limits, malformed clients, and backpressure;
- owning-thread dispatch and proof that no IPC path runs on a realtime callback;
- revision conflicts, concurrent reads, exclusive leases, heartbeats, session sequencing, and UI authority;
- atomic mutation, one undo step, one revision increment, one event, and unchanged state on failure;
- cancel and deadline behavior while queued, executing, committing, completed, and rendering;
- lost responses, operation receipt lookup, duplicate IDs, receipt eviction, and uncertain outcomes;
- event replay, gaps, ring overflow, resynchronization, and slow subscribers;
- sidecar disconnect, orphan leases, app shutdown/crash, rediscovery, and explicit reattach;
- render quotas, handle confinement, cancellation, expiry, shutdown cleanup, and startup sweeping;
- sustained fault and load conditions without IPC-induced UI or audio disruption.
