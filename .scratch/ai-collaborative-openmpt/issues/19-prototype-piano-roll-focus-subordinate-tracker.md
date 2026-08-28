# Prototype Piano Roll Focus subordinate Tracker UI

Parent: ../map.md
Type: prototype
Status: open
Blocked by: 03, 10

## Question

Within Piano Roll Focus, how should the subordinate narrow Tracker present essential row, channel, event, selection, cursor, playhead, hidden-content, and tracker-only-effect context; which pointer interactions should it support without becoming a peer editor or stealing Piano Roll's keyboard focus; how should it respond to constrained widths; and how does the user return immediately to the full Tracker?

## Comments

### Prototype constraints inherited from issues 03 and 10

- Piano Roll Focus is a layout of the existing Pattern Editor, not a separate document or editor lifecycle.
- Piano Roll owns the layout focus. The narrow Tracker is a subordinate special interface, not a second peer pane with independent cursor, selection, follow, zoom, or musical state.
- It projects the same native Tracker Pattern, `PatternCursor`, `PatternRect`, playhead, and revision as the balanced workspace.
- Tracker-only effects must remain inspectable, hidden or filtered targets must not become silently editable, and a route back to the complete Tracker must remain immediately available.
- The prototype should explore information hierarchy, pointer interaction, responsive collapse, and focus behavior. It must not redesign OpenMPT document, Undo, command, or settings infrastructure.

The prototype should retain reviewable evidence separately from production code and record a human verdict before this issue is resolved.
