# Prototype Piano Roll projection and editing semantics

Parent: ../map.md
Type: prototype
Status: resolved
Blocked by:

## Question

What should the human-facing Piano Roll show and do when projecting Tracker Pattern notes, instruments, channels, explicit note termination, volume, note delay, effects, selection, playback position, and edits without pretending the data is an ordinary MIDI clip?

## Comments

### Prototype ready for human review — 2026-08-27

- Primary-source asset: branch `prototype/piano-roll-semantics`, file `.scratch/ai-collaborative-openmpt/prototypes/piano-roll-ui-prototype.html`, captured at commit `4aaf9ca5c3b37a862d8a6d228fe429268ed64bfd`.
- Run: open the single HTML file directly; it has no dependencies or persistence.
- Switch variants with the floating left/right controls, keyboard arrow keys, or `?variant=A`, `?variant=B`, and `?variant=C`.
- Variant A — **Synchronized split**: the existing Tracker Pattern and a conventional time-horizontal Piano Roll remain visible side by side, with linked selection and projection warnings.
- Variant B — **Focused canvas**: the Piano Roll is the primary editor, with channel navigation, an event inspector, and separate volume / note-delay / effect lanes.
- Variant C — **Tracker-flow roll**: time moves downward and pitch moves right, preserving OpenMPT's existing playback and row-reading direction.
- All variants use the same in-memory pattern state and distinguish explicit note termination, implicit continuation, note delay, tracker-only effects, channel takeover, selection, playback position, and revision.
- Browser QA covered all three variants, URL/keyboard-style variant switching, note selection, playback state, and edit-mode state. No console errors were observed.
- Prototype-only caveat: the expanded full-state drawer covers part of the lower-right canvas; collapse it while judging editing space. It is not proposed as production UI.

The human verdict was supplied on 2026-08-27 and is recorded below.

## Answer

Adopt **Variant A's synchronized split workspace** as the primary Piano Roll design. The Tracker Pattern remains visible beside a conventional time-horizontal Piano Roll, and both surfaces edit the same canonical pattern data.

**The canonical Tracker Pattern model, `ModCommand` event semantics, existing Pattern edit algorithms, and Pattern undo/redo reuse the original OpenMPT implementation.** The Piano Roll adds a synchronized projection and interaction adapter over those implementations; it does not introduce a parallel note-duration model, duplicate Pattern storage, or a separate undo history.

Add a **Piano Roll focus layout option** derived from Variant B rather than keeping Variant B as a separate editor:

- horizontally narrow the Tracker pane;
- simplify the narrowed Tracker presentation to the essential row and event context needed for orientation and precise tracker-only inspection;
- horizontally expand the Piano Roll into the released space;
- retain linked selection, playback position, scrolling context, and immediate access to the full Tracker view;
- allow switching between the balanced synchronized split and the Piano Roll focus layout without changing musical data or selection.

The Piano Roll projection uses these semantics:

- pitch is vertical and pattern time is horizontal;
- channel and instrument remain explicit attributes rather than becoming DAW-style tracks implicitly;
- explicit note-off / cut / fade events are visually distinct from durations that are only bounded by a later tracker event;
- note delay and graphically useful volume or effect data may appear in auxiliary lanes or markers;
- tracker-only effects that cannot be represented faithfully remain visible as inspectable markers and continue to be editable through the Tracker or event properties;
- editing a note edge creates or changes tracker events; it does not introduce a parallel MIDI-duration model;
- selection and edits are synchronized bidirectionally and participate in the same document revision and undo operation.

Variant C's time-down layout is not part of the primary direction. It may be reconsidered later only as an experimental view, not as an initial implementation requirement.

The full A/B/C prototype is retained on `prototype/piano-roll-semantics` as the primary evidence for this decision and is removed from the main working tree so throwaway variant code cannot be mistaken for production UI.
