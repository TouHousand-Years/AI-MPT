# Prototype Piano Roll Focus subordinate Tracker UI

Parent: ../map.md
Type: prototype
Status: resolved
Blocked by: 03, 10, 22, 25

## Question

Within Piano Roll Focus, how should the subordinate narrow Tracker present essential row, channel, event, selection, cursor, playhead, hidden-content, and tracker-only-effect context; which pointer interactions should it support without becoming a peer editor or stealing Piano Roll's keyboard focus; how should it respond to constrained widths; and how does the user return immediately to the full Tracker?

## Comments

### Route correction — 2026-08-29

Piano Roll Focus remains a valid refinement, but it follows the basic Balanced
Piano Roll edit slice and the first complete AI proposal loop. This prevents a
secondary layout optimization from becoming the initial implementation entry
point.

### Prototype constraints inherited from issues 03 and 10

- Piano Roll Focus is a layout of the existing Pattern Editor, not a separate document or editor lifecycle.
- Piano Roll owns the layout focus. The narrow Tracker is a subordinate special interface, not a second peer pane with independent cursor, selection, follow, zoom, or musical state.
- It projects the same native Tracker Pattern, `PatternCursor`, `PatternRect`, playhead, and revision as the balanced workspace.
- Tracker-only effects must remain inspectable, hidden or filtered targets must not become silently editable, and a route back to the complete Tracker must remain immediately available.
- The prototype should explore information hierarchy, pointer interaction, responsive collapse, and focus behavior. It must not redesign OpenMPT document, Undo, command, or settings infrastructure.

The prototype should retain reviewable evidence separately from production code and record a human verdict before this issue is resolved.

### UI prototype ready for human review — 2026-08-30

- Review-time copy: `../prototypes/piano-roll-focus-subordinate-tracker-ui-prototype.html`; it was a dependency-free single HTML file and was removed from main after the verdict. The captured copy remains on the evidence branch below.
- Primary-source evidence: branch `prototype/piano-roll-focus-subordinate-tracker-ui`, commit `34df7af1832ea724437425b18914458e482c87d7`.
- Switch with the floating arrows, keyboard Left/Right, or `?variant=A`, `?variant=B`, and `?variant=C`. Use the Wide/Medium/Narrow controls to inspect constrained-width behavior.
- Variant A — **Dense context strip**: a persistent narrow row-by-channel Tracker projection keeps the most raw-cell detail visible and supports pointer selection/drag without taking keyboard focus.
- Variant B — **Minimal event rail**: a very narrow rail shows event presence, channel, playhead, selection, and Tracker-only markers; clicking a marker opens a transient read-only inspector.
- Variant C — **Bottom context shelf**: the Piano Roll receives the full window width while a subordinate horizontal neighborhood below it exposes nearby rows, cells, effects, selection, and playhead context.
- Every variant uses the same in-memory cursor, native-rectangle-shaped selection, playhead, Pattern revision, and Tracker events. Pointer operations in subordinate Tracker surfaces preserve Piano Roll focus. `Return to full Tracker` opens a complete Tracker surface without changing cursor or selection; Escape returns immediately.
- The full state drawer updates after every interaction. Static smoke checks passed for JavaScript syntax, all three variants, the URL switcher, state disclosure, subordinate-focus hooks, and the full-Tracker route.
- This prototype asks only which information hierarchy and responsive layout should be retained. It does not establish native MFC layout behavior, scrolling performance, rendering correctness, real Pattern mutation, or keyboard-command routing.

### Human verdict — 2026-08-30

The owner selected **Variant B — Minimal event rail** without requesting a hybrid with the other variants.

## Answer

Use Variant B's **minimal event rail** as the subordinate Tracker in Piano Roll Focus. It remains a narrow projection inside the existing Pattern Editor while the Piano Roll occupies the primary editing area and retains keyboard focus.

The event rail presents only the safety and orientation context needed beside the Piano Roll:

- row numbers and the shared playhead;
- channel-coloured event glyphs for the currently visible contiguous channel range;
- the shared cursor and native `PatternRect` selection;
- explicit markers and a count for Tracker-only or otherwise hidden detail;
- a permanently available route to the complete Tracker.

Pointer interaction stays deliberately limited. Clicking a row or event glyph navigates the shared native cursor and selection without giving the rail an independent focus or state. Dragging may extend the same native rectangular selection, but cannot create an arbitrary event set. Clicking a Tracker-only marker opens a transient **read-only** inspector containing its row, channel, raw command, and interpreted meaning. The rail and inspector do not accept Tracker keyboard editing, direct cell mutation, independent scrolling, zoom, Follow Song, cursor, or selection. Escape closes the inspector while Piano Roll focus remains active.

At constrained widths, remove labels and padding before shrinking event glyphs. The row, playhead, current cursor/selection, Tracker-only warning, and complete-Tracker route remain visible. If all visible-channel glyphs cannot fit, the event portion may scroll horizontally and must minimally reveal the cursor or selected target; it must not aggregate hidden events into an apparently editable cell or overwrite saved layout geometry.

`Return to full Tracker` leaves Piano Roll Focus and immediately restores the complete native Tracker surface at the same Pattern, semantic time, cursor, selection, playhead, and revision. Returning to Piano Roll Focus preserves that shared state and restores Piano Roll keyboard focus.

The HTML prototype is design evidence only. Production work must implement this contract through the native Pattern Editor and existing Pattern state; the prototype itself is not promoted into production code and does not prove MFC layout, rendering, performance, or keyboard routing.

The full A/B/C primary source remains on branch `prototype/piano-roll-focus-subordinate-tracker-ui` at commit `34df7af1832ea724437425b18914458e482c87d7` and is removed from the main working tree.
