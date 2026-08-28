# OpenMPT for AI

An OpenMPT-derived music tracker that adds a human-facing piano-roll editor and exposes composition capabilities to AI collaborators without changing the tracker's fundamental identity.

## Language

**Upstream Baseline**:
The OpenMPT version from which this project begins and with which its initial behavior and file handling remain compatible. Later OpenMPT releases are not automatically part of this compatibility promise.
_Avoid_: Latest OpenMPT, upstream-compatible fork

**Tracker Pattern**:
A musical structure indexed by rows and channels that contains notes, instruments, volume data, effects, and other tracker events. It remains the canonical editable music representation.
_Avoid_: MIDI clip, piano-roll clip

**Piano Roll**:
A human-facing alternative editor for Tracker Pattern data, organized primarily by time and pitch. It is not a separate music model; an AI-oriented rendering of its pitch-time plane accompanies Score Context as visual evidence.
_Avoid_: AI preview, MIDI model

**Score Context**:
A revision-bound symbolic account of the project or fragment presented to an AI collaborator. It remains the canonical musical account even when accompanied by a Piano Roll rendering or listening opinion.
_Avoid_: Piano-roll context, visual score

**Audio Preview**:
A time-bounded audio file rendered from the same project revision and musical range as a Score Context. It is intended for the Mini Audio Reviewer rather than being treated as live project state.
_Avoid_: Live project state

**Mini Audio Reviewer**:
A subordinate multimodal AI collaborator that reviews Score Context, Audio Preview, and Piano Roll evidence and returns a natural-language listening opinion. It cannot change the project, and its response is not a structured musical fact.
_Avoid_: Audio editor, autonomous composer

**Musical Range**:
A revision-bound structural span of tracker music resolved through sequence, Order occurrence, Pattern rows, and focus channels. It does not follow later UI selection or project changes.
_Avoid_: Current selection, clip

**Context Package**:
An immutable delivery unit that binds canonical Score Context, Audio Preview, Piano Roll evidence, and a Mini Audio Reviewer opinion to one document revision and Musical Range. It exists only when every required component completes successfully.
_Avoid_: Prompt dump, partial context

**Change Proposal**:
A revision-bound set of intended musical or project edits that can be inspected before it becomes part of the project.
_Avoid_: AI command, direct edit

**Review Unit**:
The smallest semantically coherent part of a Change Proposal that a human may accept or reject independently. Operations that must stay together for musical meaning or transaction safety belong to one Review Unit.
_Avoid_: Individual edit, checkbox group

**Application Capability**:
A meaningful OpenMPT-derived query, edit, or operation made available consistently to human-facing interfaces and AI integrations.
_Avoid_: GUI action, menu automation

**Capability Mode**:
A presentation scope that gives an AI collaborator one coherent family of Application Capabilities and its supporting guidance. It changes which tools are visible, not the OpenMPT workspace or the atomic boundary of a Change Proposal.
_Avoid_: UI mode, editor focus

**Agent Edit Session**:
The visible, cancellable, time-bounded period in which an AI collaborator holds an exclusive project-wide write lease for one edit-render-review loop. It ends by completion, abort, or handoff to human review; human reading and audition remain available throughout it.
_Avoid_: Background lock, AI ownership

**MCP Sidecar**:
A local, client-supervised, stateless process that translates MCP resources and tools into the protocol-neutral Application Capability contract of running OpenMPT project instances. It does not own project state or execute inside the UI or audio core.
_Avoid_: OpenMPT automation server, background project owner

**Operation Receipt**:
A bounded, document-lifetime record of a mutation request's identity, payload hash, result, committed revision, and undo provenance. It lets a collaborator determine an uncertain outcome without risking duplicate project edits.
_Avoid_: Project history, retry token

**Release Unit**:
The version-locked application, MCP Sidecar, required helpers, runtime resources, and attribution material distributed and upgraded together as one product release. Its components are not independently supported packages.
_Avoid_: Sidecar package, application bundle
