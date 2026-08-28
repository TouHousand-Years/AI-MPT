# AI-Collaborative OpenMPT: implementation-ready direction

Label: wayfinder:map
Status: open

## Destination

An implementation-ready product and architecture specification for an independent OpenMPT-derived tracker that adds a synchronized piano-roll editing view and exposes broad, reviewable composition capabilities plus symbolic, optional visual, and audio-reviewed context through MCP.

## Notes

- Domain: Windows desktop music tracker, C++/MFC OpenMPT codebase, local MCP integration, human-in-the-loop AI composition.
- Consult `grilling`, `domain-modeling`, `research`, and `prototype` as indicated by each ticket.
- The Piano Roll is primarily a human editor and edits the existing Tracker Pattern model. Score Context remains the canonical AI representation; every Context Package also contains an AI-oriented Piano Roll for the Mini Audio Reviewer, while a user preference controls whether the Main Agent sees it.
- AI changes are proposal-first, revision-bound, atomic, auditable, and undoable. Read-only analysis, preview generation, and transport controls may use a lower approval tier.
- AI audition uses bounded offline rendering for a Mini Audio Reviewer, whose natural-language opinion can accompany Score Context; the Main Agent does not require native audio input in the first release.
- Prefer a local MCP sidecar and a shared Application Capability layer over GUI automation or embedding AI protocol handling in the audio/UI core.
- **Reuse the original OpenMPT implementation** for the canonical Tracker model and event semantics, format validation and I/O, existing Pattern edit algorithms, domain-local undo/redo, playback and offline rendering, Plugin Bridge/helpers, portable-mode primitives, and upstream package/license materials. New Piano Roll, Application Capability, AI/MCP, transaction, review, and independent-distribution code wraps or adapts those implementations rather than replacing them.
- The project is an independent open-source derivative. It targets compatibility with the Upstream Baseline chosen when development begins, but does not promise complete compatibility with later OpenMPT releases.
- Preserve existing OpenMPT workflows and tracker identity; the new editor and AI collaboration are additive.
- Tracker docs are Local Markdown. Default repository label is `triage`; this map uses the required `wayfinder:map` label.

## Decisions so far

<!-- Resolved child-ticket pointers are appended here. -->
- [Score Context representation](issues/04-research-symbolic-score-context.md): use revision-bound sparse OpenMPT semantic JSON as canonical; clipboard text is a compact projection, while MIDI and MusicXML are explicitly lossy derived views.
- [Upstream Baseline and source workflow](issues/01-pin-upstream-baseline.md): pin canonical SVN `r25644` (GitHub mirror locator `0eafb124...`), preserve full imported history/licenses, and selectively sync by SVN revision on tested integration branches.
- [Piano Roll projection and editing semantics](issues/03-prototype-piano-roll-semantics.md): use a synchronized Tracker/Piano Roll split as the default, with a focus layout that narrows and simplifies Tracker context to expand the time-horizontal Piano Roll; both remain projections of the same tracker data and undo/revision state.
- [OpenMPT capability inventory](issues/02-inventory-application-capabilities.md): place a new typed facade above `CModDoc` / `CSoundFile`; legacy operations migrate only after they gain immutable queries, revision checks, atomic undo, thread ownership, and post-commit events.
- [Shared Application Capability boundary](issues/05-define-shared-capability-boundary.md): expose a closed-loop composition surface through base, Pattern, Order, and audition modes with focused Skills; AI edits may span modes atomically, offline listening is delegated to a non-editing Mini Audio Reviewer, and human writes are frozen only during a visible Agent Edit Session.
- [Local MCP sidecar IPC and lifecycle](issues/06-choose-sidecar-ipc-lifecycle.md): use one client-supervised stateless sidecar over stdio and one same-user named-pipe endpoint per app process, with explicit document attachment, owning-thread capability dispatch, revision/event resynchronization, exclusive expiring write leases, atomic cancellable requests, operation receipts, bounded offline resources, and authoritative UI cancellation.
- [AI Score Context and multimodal evidence contract](issues/07-choose-ai-context-contract.md): assemble one immutable, range- and revision-bound Context Package from lossless semantic Score Context, bounded full-mix audio, an AI-oriented Piano Roll, and an authorized text-image-audio Reviewer opinion; publish it only when every required component succeeds.
- [Reviewable AI change workflow](issues/08-prototype-reviewable-change-workflow.md): partition an immutable revision-bound Change Proposal into semantically indivisible Review Units, compare the complete proposal and accepted subset consistently in Tracker and Piano Roll, bind audition and high-impact confirmation to that exact subset, and apply it atomically as one revision and undo step.
- [Independent project distribution](issues/09-choose-project-distribution-model.md): ship an independently named BSD-3-Clause derivative as one version-locked application/Sidecar Release Unit, initially through unsigned x64 Windows installer, portable, and source Developer Preview artifacts on GitHub, with strict user-data and baseline-module compatibility boundaries.

## Implementation path

- [Choose the independent product identity](issues/11-choose-independent-product-identity.md).
- [Bootstrap the pinned upstream source tree](issues/12-bootstrap-upstream-source-tree.md).
- [Establish the reproducible build and CI baseline](issues/13-establish-reproducible-build-ci.md).
- [Package the installer, portable application, and MCP Sidecar](issues/14-package-installer-portable-sidecar.md).
- [Audit release licenses and source distribution](issues/15-audit-release-licenses-source.md).
- [Verify compatibility and distribution lifecycle](issues/16-verify-distribution-compatibility-lifecycle.md).
- [Publish the first Developer Preview](issues/17-publish-developer-preview.md).
- [Harden post-preview distribution and maintenance](issues/18-harden-post-preview-distribution.md).

## Not yet specified

- Exact code seams and module boundaries for the shared Application Capability layer after the Upstream Baseline and capability inventory are known.
- Complete MCP resource and tool catalogue and concrete payload schemas beyond the protocol/lifecycle policy in issue 06 and the context-specific sub-contract in issue 07.
- Plugin, sample, filesystem, and export security rules for capabilities that cross the project boundary.
- App-side broker and sidecar implementation against the issue 06 thread, transaction, event, cancellation, discovery, and bounded-resource contract.
- IPC integration, MCP conformance, load, and fault-injection verification; tracker/piano-roll equivalence, file compatibility, and musical-regression coverage also remain to be implemented.
- The implementation work tracked in issues 11 through 18, including formal identity, source bootstrap, build/CI, packaging, licensing, compatibility evidence, preview publication, and post-preview hardening.
- Whether later phases should model tracker-specific playback semantics such as NNA, note delay, retrigger, and inferred note duration more deeply in the Piano Roll.

## Out of scope

- Replacing the tracker editor or changing OpenMPT's fundamental tracker-oriented product identity.
- Guaranteeing compatibility with every future OpenMPT release or continuously mirroring upstream development.
- Treating a Piano Roll rendering or Reviewer opinion as canonical musical truth; both remain evidence accompanying the authoritative Score Context.
- Cloud-hosted multi-user collaboration or a remote MCP service in the initial direction.
- Unreviewed arbitrary filesystem, process, or network access by AI tools.
- Seeking acceptance of these changes into upstream OpenMPT as a condition of success.
