# Prototype the local MCP round trip

Parent: ../map.md
Type: prototype
Status: resolved
Blocked by: 06, 21, 23

## Question

What is the thinnest local app-to-Sidecar-to-MCP-client prototype that attaches
to one running application and one explicit document, obtains the chosen
revision-bound Pattern Score Context, reports understandable failures, and
demonstrates where owning-thread dispatch belongs without first implementing
multi-instance discovery, write leases, Operation Receipts, packaging, or a
complete protocol catalogue?

## Comments

The prototype should preserve the chosen process boundary while deliberately
leaving mature-system hardening outside the first round trip.

## Answer

### Prototype question and scope

The first useful round trip needs only one client-launched Sidecar, one known
running OpenMPT application, one explicitly addressed open document, and the
`get_pattern_context` query chosen in issue 23. It does not need general
multi-instance discovery, a capability catalogue, candidate writes, retained
occupancy, leases, receipts, events, packaging, or recovery machinery to fix
the responsibility boundary.

The logic prototype asks whether the following minimum pipeline is complete
and understandable:

1. An MCP client launches the stateless Sidecar over stdio.
2. The Sidecar connects to the one application endpoint and completes a
   minimal compatible handshake, without selecting a document implicitly.
3. The Sidecar explicitly attaches the supplied document ID.
4. An MCP `tools/call` for `get_pattern_context` is translated into a small,
   protocol-neutral app request containing the request ID, operation,
   document ID, Pattern, and requested row/channel range.
5. The app-side IPC broker validates and queues the envelope but never reads
   `CModDoc`, `CSoundFile`, or `CPattern` directly.
6. The document owning thread rechecks that the explicitly addressed document
   is still live, captures the immutable Pattern snapshot, and returns the
   sparse semantic Score Context.
7. The Sidecar maps that app result to MCP structured content without
   inventing additional document-selection or music semantics.

The thinnest first implementation may receive its one app endpoint through an
explicit prototype configuration rather than implementing runtime-directory
discovery. Explicit document attachment remains required even when only one
document is open. This keeps the process and owning-thread seams in their final
direction while removing mature lifecycle work from the spike.

### State and failure result

The accepted state model exposes the complete relevant state after every
action: application and document lifetime, Sidecar launch and app connection,
explicit document attachment, current request phase, owning-thread queue, MCP
request, translated app envelope, and final result.

Four guided cases establish the minimum behavior:

1. **Complete read round trip:** a valid request stops in the owning-thread
   queue, then returns Score Context only after document-thread dispatch.
2. **No explicit attachment:** the Sidecar returns `notAttached` rather than
   guessing the foreground or sole document.
3. **Document closes while queued:** dispatch returns `documentGone`; the old
   ID never drifts to another document.
4. **Broker attempts a direct model read:** the request is rejected with
   `owningThreadRequired`, making the dispatch seam visible rather than merely
   documenting it.

Failures identify their layer and stable error code in both the readable state
and the wire result. They do not change attachment or retarget the request.
The initial actual spike needs only these three business failures plus
`instanceGone`, connection/schema failure, and a safe `internalError`; the
larger issue-06 taxonomy remains deferred until corresponding behavior exists.

### Owner verdict and primary evidence — 2026-08-30

The owner manually exercised the prototype and confirmed that its logic is
correct. The validated primary source is retained outside main:

- branch: `prototype/local-mcp-round-trip-logic`;
- commit: `6a1b1ce287fae0ee6b2a081f7f915c0fd88607bc`;
- file: `.scratch/ai-collaborative-openmpt/prototypes/local-mcp-round-trip-logic-prototype.html`;
- run: open the self-contained HTML file directly; it has no dependencies or
  persistence.

The pure state module also passed a local smoke run covering queue-before-read,
successful document-thread completion, `notAttached`, `documentGone`, and
`owningThreadRequired`.

### What this verdict does not prove

This was deliberately an in-memory logic prototype. It does **not** prove that
MCP stdio, Windows named pipes or ACLs, an MFC owning-thread dispatcher, the
Score Context serializer, or native `CPattern` capture have been implemented
or connected. It also does not establish performance, cancellation, shutdown,
write occupancy, proposal mutation, or atomic Apply behavior.

The verdict answers the issue's seam and minimum-state question: the Sidecar is
only a translator, explicit document attachment is mandatory, and the app IPC
broker must stop at validation and queueing while the document owning thread
alone captures the Pattern snapshot. Any later claim that the end-to-end
product works still requires a real native transport and application
integration check; this prototype must not be cited as that evidence.
