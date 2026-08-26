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
A human-facing alternative editor for Tracker Pattern data, organized primarily by time and pitch. It is neither a separate music model nor an AI context representation.
_Avoid_: AI preview, MIDI model

**Score Context**:
A revision-bound symbolic account of the project or fragment currently presented to an AI collaborator. It conveys musical structure directly and does not depend on a screenshot of the Piano Roll.
_Avoid_: Piano-roll context, visual score

**Audio Preview**:
A time-bounded audible rendering associated with the same project revision and musical range as a Score Context.
_Avoid_: Live project state

**Change Proposal**:
A revision-bound set of intended musical or project edits that can be inspected before it becomes part of the project.
_Avoid_: AI command, direct edit

**Application Capability**:
A meaningful OpenMPT-derived query, edit, or operation made available consistently to human-facing interfaces and AI integrations.
_Avoid_: GUI action, menu automation
