# Prototype the reviewable AI change workflow

Parent: ../map.md
Type: prototype
Status: resolved
Blocked by:

## Question

How should a human request, inspect, audition, partially accept, reject, apply, undo, and compare a revision-bound Change Proposal across tracker and Piano Roll workflows, including stale-base conflicts and high-impact operations?

## Comments

### Logic prototype ready for human review — 2026-08-28

- Review-time working asset, now retained only on the evidence branch: `.scratch/ai-collaborative-openmpt/prototypes/reviewable-change-workflow-logic-prototype.html`.
- Run: open the single HTML file directly; it has no dependencies or persistence.
- Question under test: does it feel natural to partially accept only between indivisible review units, bind inspection and audition evidence to the exact accepted subset, apply that subset as one atomic revision and undo step, reject stale proposals instead of silently rebasing them, and add a separate confirmation for high-impact units?
- Guided scenarios cover full accept plus undo, partial accept, a stale-base conflict, and a high-impact gate. Free-play controls deliberately allow illegal action attempts so the rejection and unchanged state are visible.
- The Tracker and Piano Roll panels are comparison drivers for the state model, not a proposed production visual design.
- The prototype remained in the working tree while awaiting the human verdict.

### Partial human verdict — 2026-08-28

The human collaborator confirmed every candidate workflow rule except the inspect/compare surface. The accepted logic is:

- partial acceptance occurs only between indivisible review units;
- changing the accepted subset invalidates prior audition evidence and high-impact confirmation;
- the exact accepted subset must be inspected and auditioned before one atomic apply;
- a stale-base proposal remains read-only and cannot be silently rebased or applied;
- a selected high-impact review unit requires separate confirmation;
- one successful apply advances the document revision once and creates one undo step; undo is a later revision rather than revision-number rollback;
- illegal or premature actions fail visibly and leave project state unchanged.

The original comparison panel was rejected because its Tracker rows were static and its Piano Roll did not visibly distinguish a partial subset. The corrected prototype renders concrete baseline-to-proposal event differences from the same Review Units in both projections and adds an explicit accepted-subset comparison step.

### Final human verdict and primary evidence — 2026-08-28

The human collaborator confirmed the corrected inspect/compare behavior on 2026-08-28. The complete prototype is retained outside main as primary evidence:

- branch: `prototype/reviewable-change-workflow`;
- commit: `5d769cc50e24c674ff9b469390517c6645b79b77`;
- file: `.scratch/ai-collaborative-openmpt/prototypes/reviewable-change-workflow-logic-prototype.html`;
- run: open the single HTML file directly; it has no dependencies or persistence.

## Answer

### Proposal construction and handoff

**Supported Pattern operations and domain-local undo/redo reuse the original OpenMPT implementation through the Application Capability layer.** The project adds the private candidate state, Change Proposal and Review Unit model, accepted-subset identity, cross-domain transaction coordination, revision/event semantics, and Operation Receipt. Existing OpenMPT undo buffers are implementation material for the final commit, not a substitute for this review workflow.

A human request names the goal and scope; the application atomically captures the target document identity, base revision, and informing Context Package. The Agent works under the visible, cancellable Agent Edit Session from issue 05 and builds a private candidate state rather than exposing intermediate edits as project state. Request cancellation discards that candidate and leaves the document unchanged.

Successful generation hands off one immutable Change Proposal and releases Agent edit authority. The proposal records its ID, document identity, base revision, informing Package, generator provenance, creation time, affected ranges and domains, semantic operations, impact classifications, and preview evidence. Human navigation, comparison, and audition remain available throughout review; human writes may resume after handoff, with any later document revision making the proposal stale.

### Review Units and partial acceptance

Every proposal is partitioned into **Review Units**, the smallest semantically coherent groups that may be accepted or rejected independently. Operations that must remain together for musical meaning, referential validity, or transaction safety are one unit; the UI never offers an operation-level split that would create an invalid candidate. Each unit identifies its operations, affected Tracker and Order locations, impact tier, rationale, and inspection requirements.

All units start selected. The human may reject individual units or the entire proposal. The application derives an exact accepted-subset identity from the selected unit IDs and their immutable contents. Changing that subset invalidates its prior audition and any high-impact confirmation. Inspection evidence remains attached to the unchanged units it actually covered.

The accepted subset is still one transaction, not a series of per-unit commits. Empty subsets cannot be applied. Rejected units never enter project state or the undo record.

### Inspection and comparison

The review surface always offers three explicit objects: the base revision, the complete original proposal, and the current accepted subset. Tracker and Piano Roll are synchronized projections of the same semantic diff, not separately computed interpretations. Both show additions, removals, replacements, affected ranges, Review Unit identity, selection state, and linked focus.

Piano Roll comparison covers pitch-time geometry and graphical events it can represent faithfully. Tracker comparison remains authoritative for tracker-only effects, Order operations, and other data that the Piano Roll cannot express. Each Review Unit declares whether inspection in either projection is sufficient or whether Tracker inspection is required. Marking a view inspected records the exact proposal, unit, view kind, and diff version that was seen.

A stale proposal remains available for read-only comparison. The UI distinguishes base, proposed, and current document states rather than presenting old music as current. Applying or auditioning a stale proposal is forbidden; the human may explicitly request a new proposal from the current revision, while the old proposal remains identifiable as superseded for comparison and audit.

### Audition and high-impact gates

Audition is generated from the exact accepted subset applied to a private candidate snapshot. Its provenance binds document identity, base revision, proposal ID, accepted-subset hash, range, and artifact hashes. It uses the all-or-nothing render, Piano Roll, and Mini Audio Reviewer requirements from issue 07, but it is preview evidence rather than a document revision or mutation authority.

Every non-empty accepted subset must be inspected and must complete this matching audition before apply. Selection changes invalidate the preview instead of silently reusing evidence for different music.

The capability contract assigns an impact tier to every Review Unit. Destructive removal, broad structural change, and cross-domain Order/Pattern operations are initially high impact. If any selected unit is high impact, the UI requires a separate, explicit confirmation after the matching comparison and audition. That confirmation names the affected units and becomes invalid when the accepted subset changes.

### Apply, reject, undo, and failure

Apply is a deliberate human action. The application reacquires exclusive edit authority, verifies document identity and exact `expectedRevision`, revalidates every selected operation and format limit, and confirms that inspection, audition, and high-impact evidence match the accepted-subset identity. It then commits the whole subset or nothing.

One successful apply creates exactly one document revision increment, one undo step, one capability event, and one Operation Receipt that links the proposal, accepted and rejected Review Units, base and committed revisions, and semantic payload hash. Validation, deadline, cancellation, resource, or commit failure leaves the document unchanged and exposes a typed failure; no unit is reported as partially applied.

Rejecting the whole proposal changes no document state and releases proposal-only resources. After apply, ordinary human undo reverses the entire accepted subset in one step. Undo is itself a later document mutation with a new revision and event; revision identifiers never move backward. The proposal, receipt, and retained snapshots continue to support comparison for their bounded document-lifetime audit period.

### Required verification

Implementation is incomplete until state-machine, integration, and visual-equivalence tests cover:

- generation completion, cancellation, failure, handoff, and unchanged document state before apply;
- unit independence, inseparable cross-domain units, empty selection, full reject, and exact accepted-subset hashing;
- invalidation of audition and confirmation after every selection change;
- synchronized Tracker/Piano Roll diffs for added, removed, replaced, delayed, tracker-only, and Order events;
- per-unit inspection requirements and proof that a visually omitted operation cannot be approved through Piano Roll alone;
- exact-subset preview identity and all-or-nothing failure of rendering or review;
- high-impact classification, explicit confirmation, and confirmation invalidation;
- revision conflicts before preview and apply, stale read-only three-way comparison, and explicit regeneration without silent rebase;
- one atomic apply, one revision increment, one undo step, one event, one receipt, and unchanged state on every failure path;
- lost apply responses resolved through Operation Receipts without duplicate application;
- whole-proposal rejection, post-apply undo, later revision semantics, resource expiry, and audit redaction.
