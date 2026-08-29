# Define Piano Roll workspace state and synchronization

Parent: ../map.md
Type: grilling
Status: resolved
Blocked by: 05, 09

## Question

For the balanced synchronized split and Piano Roll focus layouts, which selection, cursor, playback, scroll, zoom, channel-filter, auxiliary-lane, splitter, and simplification states are shared with the Tracker, which remain view-local, which persist globally or per document, what essential Tracker information must remain visible when its pane is narrowed, and how does persistence honor issue 09's prohibition on implicit private module metadata?

## Comments

### Scope correction — 2026-08-28

The workspace contract extends OpenMPT's existing Pattern view rather than replacing application-level behavior. Document/view lifetime, `PatternCursor`, `PatternRect`, Pattern undo/redo, MultiView, Follow Song, settings storage, Save As, and multi-instance settings coordination remain owned by the pinned OpenMPT baseline. The Piano Roll adapts those mechanisms and adds only the state required by the synchronized projection.

Current source evidence confirms existing `PatternViewState`, per-view save/load, and per-song window-state persistence. Exact seams and names must be checked against the checked-in Starting Snapshot before editing them; this issue does not authorize a parallel JSON store, content-signature identity, revision-remapping system, or replacement document lifecycle.

## Answer

Embed the Piano Roll workspace inside the existing Pattern Editor view. It is an additive projection over the native Tracker Pattern state, not a new document type or independent editor lifecycle. Preserve OpenMPT MultiView: each native Pattern view may contain one workspace, while the extension itself does not create extra windows.

### State ownership and synchronization

| State | Contract |
| --- | --- |
| Pattern, order, cursor, and selection | Reuse the native Pattern view state. `PatternRect` remains the single authoritative selection; the Piano Roll does not introduce an arbitrary event-set selection. |
| Piano Roll selection gestures | Translate clicks, Ctrl-clicks, drags, and lassos into visible movement of `PatternRect` boundaries. Preview the actual Tracker rectangle before committing; never silently substitute a bounding rectangle that selects undisclosed events. |
| Playback and Follow Song | Reuse the Pattern view's single playhead and Follow Song state. Tracker follow remains unchanged. Piano Roll follow offers **page advance** and **continuous centering**, with page advance as the default. |
| Scroll and viewport | Do not pixel-lock Tracker and Piano Roll. Synchronize semantic time/channel context and minimally reveal an explicitly navigated target. Playback does not chase pitch; empty rows and tracker-only events preserve the current pitch viewport. |
| Zoom and layout geometry | Piano Roll zoom, splitter geometry, and auxiliary-lane geometry are view state. Balanced and Focus layouts keep separate geometry/zoom, but switching layouts preserves the current semantic time and pitch center. |
| Channel filter | One workspace-level, contiguous channel range applies to both Tracker and Piano Roll. It changes visibility only, never playback, mute state, Pattern data, or saved module content. |
| Auxiliary lanes | Lanes project native Tracker fields. Their order, visibility, and height are view state; unsupported lanes retain their configuration but appear unavailable or temporarily collapsed and never create parallel musical fields. |

`PatternRect` compatibility constrains the initial Piano Roll selection model. A lasso sets the minimum/maximum row and channel boundaries. Ctrl-click outside the rectangle extends the nearest boundary; Ctrl-click inside moves the nearest boundary inward. Boundaries cannot cross, ambiguous results are previewed, and mouse release commits the change. Operations that cannot faithfully handle the resulting native rectangle or tracker-only events must stop rather than partially or silently edit it.

The channel filter only permits a contiguous range so a native rectangular selection cannot cross hidden intermediate channels. If applying a filter would hide currently selected events, preview the reduced rectangle and require confirmation, then crop the selection to the visible range. Hidden events are not selected, cannot be copied, and are treated as nonexistent by export without an additional warning. Re-showing a channel does not restore an earlier selection.

Selecting or navigating to a pitched event outside the Piano Roll viewport minimally scrolls the pitch axis to reveal it. Playback alone does not move the pitch viewport. Clicking an auxiliary lane maps the native cursor/selection field boundary to the corresponding Tracker column; moving the Tracker cursor highlights an already visible matching lane but does not open a lane the user hid.

### Layout behavior

The first unsaved workspace state uses the **Balanced** layout with Tracker as the active pane. The Piano Roll time position follows the native cursor; its initial pitch position uses the current note when available and otherwise OpenMPT's current base octave. No auxiliary lane opens automatically.

In Balanced layout, clicking Tracker or Piano Roll determines the active pane. Switching into **Piano Roll Focus** makes Piano Roll the focus; returning to Balanced retains Piano Roll focus. Piano Roll Focus treats its narrow Tracker as a subordinate special interface rather than a peer editor pane. The exact information hierarchy, pointer interaction, responsive presentation, and route back to the full Tracker are deliberately assigned to [issue 19](19-prototype-piano-roll-focus-subordinate-tracker.md).

When space is insufficient, temporarily collapse auxiliary lanes first, then compress the Piano Roll viewport, then allow non-active channels to scroll horizontally while retaining the narrowed Tracker's safety context. Responsive constraints do not overwrite saved geometry.

### Persistence

Extend the pinned baseline's existing per-view/per-song state mechanism. Persist the active layout/pane, layout-specific splitter geometry, Piano Roll zoom, channel filter, follow presentation mode, and auxiliary-lane stack. Native cursor, selection, exact scroll position, document identity, Save As behavior, configuration paths, error handling, and settings concurrency continue to follow OpenMPT's existing lifecycle. Do not write workspace metadata into module files or module directories.
