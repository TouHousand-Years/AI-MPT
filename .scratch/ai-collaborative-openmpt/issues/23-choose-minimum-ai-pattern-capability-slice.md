# Choose the minimum AI Pattern capability slice

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by: 05, 20

## Question

Which smallest immutable Pattern queries, Score Context range, proposal
operation or operations, validation rules, document-revision check, atomic
apply behavior, and undo result are required for the first owner workflow, and
which broader Application Capabilities should remain unavailable until another
observed workflow needs them?

## Comments

This ticket applies the shared capability direction slice-first. It must not
turn the inventory of everything OpenMPT can do into an implementation
prerequisite.

The capability slice was grilled in rounds and confirmed by the owner on
2026-08-30. It specializes the broader directions from issues 04, 05, 08, and
20 for one local, single-Pattern workflow. In particular, revision identity is
kept inside the application contract rather than exposed to the Agent in the
first version.

## Answer

### Minimum AI capability surface

Expose only five Pattern-mode tools through the protocol-neutral Application
Capability boundary:

1. `get_pattern_context` reads an immutable Pattern snapshot. Both reads and
   writes take a short occupancy for their call; `occupy=true` promotes it to a
   retained cross-call session and returns a session token.
2. `replace_pattern_segment` replaces one contiguous row segment in one
   Tracker channel in the private candidate state.
3. `handoff_for_review` freezes the final candidate as an immutable proposal
   and releases occupancy atomically.
4. `abort_session` discards the candidate and releases occupancy atomically.
5. `release_occupancy` ends a retained read-only session. It fails if candidate
   edits exist, requiring an explicit handoff or abort instead.

Every tool result reminds the Agent to use the appropriate ending call when it
has finished. A human force-release remains higher priority than Agent calls;
it invalidates the token and candidate. Human Apply, Reject, Undo, playback,
and saving are application actions, not MCP tools.

### Immutable Pattern query and Score Context

The first occupied read binds the current document, current Pattern, initial
write envelope, and internal dependency signature. Later UI navigation or
selection changes do not retarget that session. An explicit `PatternRect` is
the initial write envelope; with no selection, the whole bound Pattern is the
envelope.

`get_pattern_context` defaults to the whole bound Pattern across all channels
and may optionally narrow the returned rows and channels. Reads are not
restricted by the write envelope, but a session can never read another
Pattern. The same query reads the current candidate by default when given a
session token and can explicitly request the immutable baseline.

The response is a sparse semantic Score Context rather than a C++ memory dump.
It contains:

- bound Pattern identity, dimensions, and exact requested row/channel range;
- the timing and Pattern-local context needed to interpret rows;
- current module format limits;
- read-only summaries and stable references for existing instruments and
  samples; and
- every non-empty Tracker cell in the range, preserving note or special-note,
  instrument, volume command/value, and effect command/parameter through stable
  semantic names plus the raw values needed for exact validation.

The first Agent contract does not expose a document revision number, candidate
revision number, or separately computed impact-range field. The opaque session
token binds the internal snapshot. A successful mutation returns the
normalized resulting cells and the actual before/after cell differences for
the requested segment; the Agent can query the complete candidate when it
needs wider feedback.

### Single-voice segment replacement

One mutation call targets exactly one Tracker channel and one contiguous row
interval. The Agent chooses the start and the submitted segment length; the
interval may be as long as the whole bound Pattern. Multiple segments or
voices require multiple calls, which accumulate in one private candidate and
one final proposal.

The payload represents complete desired raw Tracker cells for that segment,
not a sequence of per-note commands. Its transport is sparse: non-empty cells
are listed, while an omitted row means that the desired cell is empty. This
lets one call add, move, replace, and delete notes without exposing separate
note-edit or transpose tools.

Each supplied cell carries the complete note, instrument, volume, and effect
fields. The first write surface may change only:

- ordinary pitched notes, empty notes, and note-off events;
- references to instruments already present in the module; and
- ordinary volume values.

Existing effect commands, non-volume volume-column commands, note cuts, fades,
PC/PCS notes, complex delays, and all other unsupported Tracker semantics must
remain byte-for-byte equivalent in the proposed cell. A normal note may change
beside a preserved effect in the same cell. If a requested note would occupy
the same note field as an unsupported special event, the whole call fails
rather than dropping, approximating, or silently merging that event.

The Agent may choose any segment inside the granted envelope. A segment that
would expand an explicit envelope pauses that Tool call for the previously
defined non-modal OpenMPT approval. Approval enlarges the session-local
envelope and resumes the call; rejection leaves the candidate unchanged. The
global ask-each-time / always-approve preference from issue 20 still applies.

### Validation and candidate feedback

Before changing the private candidate, a segment call validates the session
and occupancy, bound Pattern, single-channel contiguous range, write envelope,
cell schema, row and channel limits, note and volume values, existing
instrument references, module-format limits, and exact preservation of every
unsupported field. The application validates structural and format integrity,
not melody quality, harmony quality, or adherence to taste.

Any failure rejects the complete call and leaves the candidate exactly as it
was. There is no clamping, coercion, silent merge, skipped cell, or partial
success. The result identifies the failing row, field, value, and reason so the
Agent can correct the whole segment and retry. Successful results return the
normalized segment and concrete cell diff without a revision or additional
impact assessment.

The service serializes calls for the single active local session. There is no
Agent-visible or required `candidate_revision`; the retained token and ordered
call processing are sufficient for this slice. Mature idempotency records,
operation receipts, and multi-client ordering remain deferred.

### Pattern-scoped stale check

The application maintains an opaque internal dependency signature for the
bound Pattern. It covers the Pattern data plus the timing, module-format state,
and referenced instrument or sample state that informed the Score Context.
Changes to any of those dependencies invalidate the session or handed-off
proposal. Changes to an unrelated Pattern do not.

During retained occupancy, human project writes, Undo, and Redo remain blocked
as specified by issue 20. After handoff, human writes may resume. Apply compares
the current Pattern dependency signature with the proposal's internal base and
reports a stale proposal instead of rebasing it. The signature is not shown to
the Agent; it receives only the resulting current, stale, or lost-session
state.

### Proposal, atomic Apply, and Undo

Repeated calls may replace the same segment while the Agent evaluates its
work. Handoff compares the final candidate Pattern with the immutable baseline
and stores one normalized final cell diff. Superseded intermediate calls are
not review units and are never replayed during Apply.

The human reviews and accepts or rejects the whole proposal. Apply reacquires
edit authority, rechecks the Pattern dependency signature, and prevalidates
every final cell. It then commits the complete diff or nothing, updates the
document once, and creates exactly one ordinary Pattern Undo step even when
the candidate was assembled through several voices and calls. Validation,
staleness, occupancy, or commit failure leaves the Pattern unchanged. Reject
also leaves it unchanged. Ordinary human Undo reverses the complete applied
proposal in one step; no AI undo or special proposal-undo capability is added.

The implementation should reuse native `CPattern` / `ModCommand` storage,
module-format checks, and `CPatternUndo` snapshots. Issue 22 proves native
single-note mutation and ordinary Pattern Undo are viable seams, but it does
not implement the Score Context serializer, occupancy, private candidate,
dependency signature, whole-proposal prevalidation, or atomic multi-segment
commit. Those are implementation work for the MCP and proposal-loop tickets.

### Explicitly unavailable

The first AI surface does not provide:

- access to another Pattern, cross-Pattern operations, Pattern creation,
  deletion, resizing, or renaming;
- Order reads or writes;
- per-note mutation tools, standalone transpose or other transform tools;
- writes to effects, note cuts, delays, PC/PCS events, or other unsupported
  Tracker semantics;
- creation or editing of instruments, samples, plugins, routing, tempo, meter,
  or project settings;
- AI playback, rendering, saving, Apply, Reject, Undo, or Redo;
- partial proposal acceptance, Mini Audio Reviewer integration, cross-domain
  transactions, remote or multi-client coordination, or a complete autonomous
  composition workflow.

These absences constrain the AI capability surface, not the owner's ordinary
Tracker, Piano Roll, playback, review, Apply, Undo, or Save As workflows.
Broader capabilities graduate only after an observed workflow requires them.
