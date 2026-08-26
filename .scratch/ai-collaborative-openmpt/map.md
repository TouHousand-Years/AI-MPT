# AI-Collaborative OpenMPT: implementation-ready direction

Label: wayfinder:map
Status: open

## Destination

An implementation-ready product and architecture specification for an independent OpenMPT-derived tracker that adds a synchronized piano-roll editing view and exposes broad, reviewable composition capabilities plus symbolic score and audio context through MCP.

## Notes

- Domain: Windows desktop music tracker, C++/MFC OpenMPT codebase, local MCP integration, human-in-the-loop AI composition.
- Consult `grilling`, `domain-modeling`, `research`, and `prototype` as indicated by each ticket.
- The Piano Roll is for humans and edits the existing Tracker Pattern model; AI receives Score Context directly and does not need a piano-roll image.
- AI changes are proposal-first, revision-bound, atomic, auditable, and undoable. Read-only analysis, preview generation, and transport controls may use a lower approval tier.
- Prefer a local MCP sidecar and a shared Application Capability layer over GUI automation or embedding AI protocol handling in the audio/UI core.
- The project is an independent open-source derivative. It targets compatibility with the Upstream Baseline chosen when development begins, but does not promise complete compatibility with later OpenMPT releases.
- Preserve existing OpenMPT workflows and tracker identity; the new editor and AI collaboration are additive.
- Tracker docs are Local Markdown. Default repository label is `triage`; this map uses the required `wayfinder:map` label.

## Decisions so far

<!-- Resolved child-ticket pointers are appended here. -->
- [Score Context representation](issues/04-research-symbolic-score-context.md): use revision-bound sparse OpenMPT semantic JSON as canonical; clipboard text is a compact projection, while MIDI and MusicXML are explicitly lossy derived views.
- [Upstream Baseline and source workflow](issues/01-pin-upstream-baseline.md): pin canonical SVN `r25644` (GitHub mirror locator `0eafb124...`), preserve full imported history/licenses, and selectively sync by SVN revision on tested integration branches.

## Not yet specified

- Exact code seams and module boundaries for the shared Application Capability layer after the Upstream Baseline and capability inventory are known.
- Complete MCP resource and tool catalogue, schemas, error model, and version-negotiation policy beyond the first implementation slice.
- Audio-preview rendering lifecycle, caching, cancellation, and synchronization with editing and playback.
- Plugin, sample, filesystem, and export security rules for capabilities that cross the project boundary.
- Threading and realtime-safety rules between UI, document editing, audio playback, rendering, IPC, and MCP requests.
- Testing strategy for tracker/piano-roll equivalence, undo transactions, file compatibility, IPC, MCP conformance, and musical regressions.
- Build, packaging, CI, migration, and release phases after the architecture and project distribution model are chosen.
- Whether later phases should model tracker-specific playback semantics such as NNA, note delay, retrigger, and inferred note duration more deeply in the Piano Roll.

## Out of scope

- Replacing the tracker editor or changing OpenMPT's fundamental tracker-oriented product identity.
- Guaranteeing compatibility with every future OpenMPT release or continuously mirroring upstream development.
- Making the Piano Roll or its screenshot part of the AI's required context.
- Cloud-hosted multi-user collaboration or a remote MCP service in the initial direction.
- Unreviewed arbitrary filesystem, process, or network access by AI tools.
- Seeking acceptance of these changes into upstream OpenMPT as a condition of success.
