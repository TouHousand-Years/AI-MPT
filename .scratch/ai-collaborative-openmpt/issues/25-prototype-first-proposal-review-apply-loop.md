# Prototype the first proposal-review-apply loop

Parent: ../map.md
Type: prototype
Status: resolved
Blocked by: 08, 22, 23, 24

## Question

How should one bounded AI Pattern proposal from the local MCP loop be shown to
the owner in synchronized Tracker and Piano Roll evidence, deliberately
accepted or rejected, checked against the expected document revision, applied
atomically, heard, and undone so that the full personal-use vertical slice is
credible without requiring partial Review Unit acceptance, mandatory Mini
Audio Reviewer output, impact tiers, or long-lived audit machinery?

## Comments

This prototype is the completion point of the first route. Its human verdict
determines which deferred safety, audio, layout, and capability questions
graduate from the map's fog.

## Answer

### Prototype question and first-slice reduction

The first proposal loop keeps the safety core from issue 08 while removing the
mature gates that issues 20 and 23 explicitly deferred. One bounded proposal
is immutable after handoff, is accepted or rejected only as a whole, is checked
against its internal Pattern dependency signature, and is committed atomically
by a deliberate human Apply. Pre-Apply audition, partial Review Units, impact
tiers, Reviewer output, receipts, and long-lived audit evidence are not Apply
preconditions.

The logic prototype uses a concrete melody-to-two-part-harmony example. Two
separate single-voice candidate calls add Ch2 and Ch3, eight Tracker cell
changes in total. They accumulate in one private candidate and normalize into
one final proposal. This fixture exercises multi-call candidate accumulation
and a multi-voice atomic commit without implying that the Agent may edit more
than one voice in one Tool call.

### Retained occupancy across Tool calls

Candidate construction begins with `occupy=true`. Both Tool calls have their
own short call-level exclusion, and retained occupancy remains active after the
first result is returned, while the Agent evaluates it and before the second
call begins. During this cross-call interval:

- human project writes, Undo, and Redo remain frozen;
- navigation, synchronized Tracker/Piano Roll inspection, and normal playback
  remain available;
- normal playback hears only the committed Pattern, never the private
  candidate; and
- the document cells and ordinary Undo stack remain unchanged.

`handoff_for_review` freezes the final candidate as an immutable proposal and
releases occupancy atomically. `abort_session`, occupancy timeout, or the
human's high-priority force release discards the private candidate and releases
the freeze without creating a proposal or modifying the Pattern. There is no
unlocked mutable-candidate state between Tool calls or after handoff.

The prototype models each individual Tool call as one atomic state transition;
it does not animate the short exclusion inside a running call. Its explicit
acceptance target is the retained state between calls and all first-slice exit
paths.

### Whole-proposal review evidence

Handoff compares the final candidate with its immutable baseline and exposes
one normalized final cell diff. Superseded intermediate calls are not replayed
or presented as review items. Before Apply, the current document remains
unchanged.

Tracker and Piano Roll are synchronized projections of that same concrete cell
diff. The Tracker view lists each row/channel coordinate alongside baseline,
proposal, and current-document cells. The Piano Roll independently renders the
proposal baseline, immutable proposal result, and current document from the
same data. Thus it visibly shows all three important moments:

- before Apply, current document equals the baseline while the proposal adds
  the harmony voices;
- after Apply, current document equals the proposal result; and
- after Undo, current document returns to the baseline while the historical
  proposal remains identifiable.

These projections are state-model drivers, not a production visual design.
The first slice has no per-voice or per-cell acceptance control. An attempted
partial acceptance fails as unsupported and changes no state. The human may
inspect either or both views, but the application does not require an
inspection checkbox or other defensive proof before Apply.

### Stale, Reject, retry, and atomic Apply

Handoff releases occupancy, so human edits may resume. If the bound Pattern or
one of the Pattern dependencies defined by issue 23 changes afterward, the
proposal becomes stale. It remains available for read-only comparison but
cannot be applied or silently rebased. Natural-language feedback starts a new
candidate from the current Pattern and produces a new immutable proposal ID.

Reject discards the complete proposal outcome without changing Pattern cells,
the Pattern dependency signature, or the Undo stack. It does not edit the old
proposal in place; a retry is a fresh session and proposal.

Apply is a human action. It reacquires edit authority, checks that the proposal
is still current, and revalidates every final cell and format constraint before
commit. It then writes the complete normalized diff or nothing. A simulated
commit failure in the prototype leaves every Pattern cell, dependency
signature, and Undo entry unchanged, and the same immutable proposal remains
available for a later retry.

One successful Apply changes the Pattern once and pushes exactly one ordinary
Pattern Undo entry even when the candidate came from multiple voice calls.
There is no AI Apply, no per-call commit, and no partial success.

### Hearing, Undo, and feedback

Pre-Apply candidate audition is not required. After Apply, the owner uses
ordinary OpenMPT playback to hear the committed result. If it is satisfactory,
the owner keeps it and follows the human Save As workflow from issue 20. If it
is unsatisfactory, one ordinary Undo restores the complete pre-Apply Pattern;
the two voice calls are not undone separately. Feedback to the Agent then
starts another proposal from the current post-Undo state.

Ordinary playback remains an observation action and never changes proposal or
occupancy state. Undo is an ordinary later document mutation, not revision
rollback and not a special AI capability.

### Guided cases and owner verdict — 2026-08-30

The accepted prototype exposes full state after every action and provides six
guided cases:

1. complete Apply, normal playback, and one-step Undo;
2. whole-proposal Reject followed by a fresh feedback retry;
3. a stale proposal after a post-handoff human Pattern edit;
4. an atomic commit failure followed by a successful retry;
5. retained occupancy across two Tool calls, permitted playback, frozen human
   writes/Undo, whole-only review, and handoff release; and
6. Agent abort and occupancy-timeout release without document mutation.

The owner first confirmed the proposal/review/Apply logic, then asked that the
previously agreed cross-Tool-call freeze be made explicit. After the retained
occupancy, abort, and timeout cases were added, the owner confirmed the updated
logic as correct.

The primary evidence is retained outside main:

- branch: `prototype/first-proposal-review-apply-loop`;
- commit: `4c5dc551dde400b6f5bf32762c96f9eceaee717b`;
- file: `.scratch/ai-collaborative-openmpt/prototypes/first-proposal-review-apply-loop-logic-prototype.html`;
- run: open the self-contained HTML file directly; it has no dependencies or
  persistence.

The pure state module passed local smoke checks for private handoff, retained
cross-call occupancy, playback while occupied, frozen human writes and Undo,
handoff/abort/timeout release, atomic Apply, one-step Undo, Reject, stale-base
failure, complete rollback, and rejection of partial acceptance.

### What this verdict does not prove

This is an in-memory logic verdict. It does **not** prove real MCP or named-pipe
transport, native occupancy, `CPattern` serialization, atomic multi-range
mutation, OpenMPT playback/Undo integration, or synchronized production review
UI. It also does not demonstrate the second harmony-to-melody workflow, actual
Agent musical quality, a result worth retaining, or Save As plus close/reopen.
Those remain required before claiming the personal-use vertical slice itself
works end to end.

No deferred mature feature graduates merely because this logic was accepted.
Partial acceptance, mandatory audition, Mini Audio Reviewer output, impact
tiers, receipts, broad fault recovery, and expanded AI capabilities remain
unavailable until real use reveals a need.
