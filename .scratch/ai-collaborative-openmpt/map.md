# OpenMPT for AI: path to a personally useful vertical slice

Label: wayfinder:map
Status: open

## Destination

A clear implementation path to a locally useful personal OpenMPT derivative in which the owner can edit Tracker Pattern data through a synchronized Piano Roll and let a local AI inspect the music and propose reviewable, undoable changes. Success is one end-to-end workflow on the owner's current development machine, not public-release readiness, reproducible distribution, or fidelity to a pinned upstream revision.

## Notes

- Domain: Windows desktop music tracker, C++/MFC OpenMPT codebase, local MCP integration, human-in-the-loop AI composition.
- Consult `grilling`, `domain-modeling`, `research`, and `prototype` as indicated by each ticket.
- The Piano Roll is primarily a human editor and edits the existing Tracker Pattern model. Score Context remains the canonical AI representation.
- AI writes retain document-revision checks, deliberate human apply, atomic mutation, and ordinary undo as product-safety principles. The first slice uses a minimal visible occupancy protocol across Agent tool calls, but does not have to implement the complete mature lease, receipt, partial-acceptance, impact-tier, or mandatory-audition machinery.
- Prefer a local MCP sidecar and a narrow Application Capability seam over GUI automation or embedding AI protocol handling in the audio/UI core. Expose only the capabilities required by the current vertical slice; do not build a complete facade in advance.
- **Reuse the existing OpenMPT implementation where it lowers effort**: Tracker data and event semantics, Pattern editing, format I/O, undo/redo, playback, and offline rendering are starting materials rather than behavior that must remain byte-for-byte compatible.
- The `r25644` source under `openmpt-original_ref/` is the project's **Starting Snapshot**. It is not an active compatibility target, a reason to duplicate OpenMPT's development environment, or a requirement for continued upstream synchronization and exact SVN verification.
- Build and debug the smallest useful x64 configuration on the owner's current machine. Toolchain pinning, clean-runner reproducibility, CI matrices, installers, portable packaging, and public release work are deferred.
- Audio review, the Piano Roll Focus layout, broader Order/Plugin capabilities, and hardening graduate only after the basic Piano Roll-to-AI loop reveals that they are the next useful decision.
- Preserve existing OpenMPT workflows and tracker identity; the new editor and AI collaboration are additive.
- Tracker docs are Local Markdown. Default repository label is `triage`; this map uses the required `wayfinder:map` label.

## Decisions so far

<!-- Resolved child-ticket pointers are appended here. -->
- [Score Context representation](issues/04-research-symbolic-score-context.md): use revision-bound sparse OpenMPT semantic JSON as canonical; clipboard text is a compact projection, while MIDI and MusicXML are explicitly lossy derived views.
- [Upstream Baseline and source workflow](issues/01-pin-upstream-baseline.md): `r25644` identifies the imported source's provenance, but the current effort treats it only as a Starting Snapshot; exact verification, compatibility guarantees, and selective-sync governance are inactive.
- [Piano Roll projection and editing semantics](issues/03-prototype-piano-roll-semantics.md): use a synchronized Tracker/Piano Roll split as the default, with a focus layout that narrows and simplifies Tracker context to expand the time-horizontal Piano Roll; both remain projections of the same tracker data and undo/revision state.
- [OpenMPT capability inventory](issues/02-inventory-application-capabilities.md): use `CModDoc` / `CSoundFile` and existing domain operations as implementation material; wrap only the operations required by each vertical slice rather than treating the inventory as an application-wide migration plan.
- [Shared Application Capability boundary](issues/05-define-shared-capability-boundary.md): keep a protocol-neutral seam above the OpenMPT document model, but introduce it capability-by-capability as the vertical slice requires rather than migrating the whole application first.
- [Local MCP sidecar IPC and lifecycle](issues/06-choose-sidecar-ipc-lifecycle.md): retain the client-supervised local sidecar direction; multi-instance discovery, leases, receipts, and comprehensive fault handling are mature-system constraints, not first-slice gates.
- [AI Score Context and multimodal evidence contract](issues/07-choose-ai-context-contract.md): keep revision-bound semantic Score Context authoritative; audio, Piano Roll evidence, and Mini Audio Reviewer output are deferred enrichments rather than prerequisites for the first AI read loop.
- [Reviewable AI change workflow](issues/08-prototype-reviewable-change-workflow.md): retain proposal-first review, exact document revision, atomic apply, human confirmation, and undo; partial acceptance, mandatory audition, and high-impact tiers may follow observed need.
- [Piano Roll workspace state and synchronization](issues/10-define-piano-roll-workspace-state.md): extend the native Pattern view and its state lifecycle; retain `PatternRect`, MultiView, Follow Song, and existing settings infrastructure while adding synchronized Piano Roll layout, viewport, filter, lane, and focus state without module metadata.
- [Initial independent product identity](issues/11-choose-independent-product-identity.md): use `OpenMPT for AI` and `OpenMPT-for-AI` only as provisional working/repository names, centralize the `OpenMPTForAI` engineering prefix and isolate its user data, provide minimal independent-derivative attribution, and defer final branding and open-source publication concerns until the owner is satisfied with a releasable version.
- [Available OpenMPT starting source](issues/12-bootstrap-upstream-source-tree.md): the imported Git tree under `openmpt-original_ref/` is sufficient to begin local development; the unperformed canonical SVN export comparison is no longer a closure gate.
- [First personally useful vertical slice](issues/20-define-first-personally-useful-vertical-slice.md): validate one local MCP-driven, single-Pattern proposal loop with melody-to-harmony and harmony-to-melody tests, single-voice candidate calls, visible cross-call occupancy, whole-proposal review and atomic apply, normal playback plus undo, and human Save As/reopen confirmation; Order AI/UI, cross-Pattern work, and full-song autonomy are deferred.
- [Smallest local build-and-run loop](issues/21-establish-smallest-local-build-run-loop.md): use VS Code with Visual Studio Build Tools 2022 to build and debug only the `vs2022win10` `Debug|x64` desktop project, retargeted to the installed Windows SDK 26100; the `vs2022win11` project requires a newer OS than this host.

## Not yet specified

- Which additional Pattern, Order, instrument, sample, plugin, filesystem, and export capabilities become useful after the first bounded Pattern workflow.
- Whether real use justifies partial Review Unit acceptance, mandatory audition, impact tiers, the complete mature lease, Operation Receipts, multi-instance routing, or broader fault-injection work beyond the first slice's minimal occupancy.
- Whether audio review should become part of the ordinary AI loop, remain an optional enrichment, or be omitted.
- Whether later phases should model tracker-specific playback semantics such as NNA, note delay, retrigger, and inferred note duration more deeply in the Piano Roll.

## Out of scope

- Replacing the tracker editor or changing OpenMPT's fundamental tracker-oriented product identity.
- Treating `r25644` as a strict compatibility target; exact SVN-export verification; reproducing the original OpenMPT development environment; or continuously mirroring upstream development.
- [Independent project distribution](issues/09-choose-project-distribution-model.md): public-product packaging and release governance are deferred until the owner explicitly chooses to publish.
- [Reproducible build and CI baseline](issues/13-establish-reproducible-build-ci.md): pinned build matrices, clean-runner reproducibility, and CI are not required for the personal-use vertical slice.
- [Installer, portable application, and Sidecar packaging](issues/14-package-installer-portable-sidecar.md): installer and portable Release Unit work are deferred.
- [Release license and source-distribution audit](issues/15-audit-release-licenses-source.md): publication-grade artifact inventory, notice generation, and source archives are deferred; existing upstream notices must still be preserved.
- [Distribution compatibility and lifecycle verification](issues/16-verify-distribution-compatibility-lifecycle.md): baseline corpora and install/upgrade/uninstall matrices are deferred; touched workflows still require proportionate local verification and user-data safety.
- [First Developer Preview publication](issues/17-publish-developer-preview.md): no public preview is part of the current destination.
- [Post-preview distribution and maintenance hardening](issues/18-harden-post-preview-distribution.md): signing, SBOMs, provenance, update channels, vulnerability processes, and baseline-uplift policy belong to a future publication map.
- Treating a Piano Roll rendering or Reviewer opinion as canonical musical truth; both remain evidence accompanying the authoritative Score Context.
- Cloud-hosted multi-user collaboration or a remote MCP service in the initial direction.
- Unreviewed arbitrary filesystem, process, or network access by AI tools.
- Seeking acceptance of these changes into upstream OpenMPT as a condition of success.
