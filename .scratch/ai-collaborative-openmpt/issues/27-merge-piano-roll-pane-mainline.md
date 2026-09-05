# 27: Piano Roll pane into the main development line

Parent: 26-implement-first-personal-vertical-slice.md
Label: ready-for-agent

**What to build:** The issue-22-reviewed Piano Roll pane — child HWND with private bitmap paint, `WS_CLIPCHILDREN`, exclusion from the Tracker pixel-scroll region, whole-Pattern pitch range, piano keyboard / pitch lanes / time ruler, channel-coloured notes with strong selected-note treatment, and explicit tracker-detail markers for unsupported content — is merged from the prototype branch into the main development line and works in the standard x64 Debug build. It stays a projection of the active native `CPattern` over the shared `PatternRect`, with one synchronized playhead and ordinary Pattern Undo.

**Blocked by:** None (can start immediately)

**Status:** ready-for-agent

## Acceptance criteria (demo to owner)

- [ ] In the mainline x64 Debug build, scrolling the Tracker up and down at least 20 times leaves the Piano Roll title, keyboard, grid, notes, legend, and state line single and intact with no repaint trails or flicker.
- [ ] Selecting a note from either surface makes the other surface follow through the shared `PatternRect`; transposing one note (drag or `+1` button) and undoing through ordinary Pattern undo returns both surfaces to the same note.
- [ ] Normal Pattern playback shows one synchronized playhead on both surfaces; stopping playback clears the playhead without residue.
- [ ] In `test.mptm`, PC/effect/volume-only content appears as explicit tracker-detail markers, never as fabricated pitched notes.
- [ ] The pane remains a projection of the active native `CPattern`: no second note store, no persistence format, no parallel undo history; availability still gated by the minimum Pattern-view client size from the prototype.
