# Prototype the synchronized Piano Roll editing slice

Parent: ../map.md
Type: prototype
Status: resolved
Blocked by: 03, 10, 20, 21

## Question

In the existing Pattern Editor's Balanced layout, what is the smallest concrete
Piano Roll prototype that lets the owner see native Pattern notes, navigate and
select them through the shared Tracker state, perform the vertical slice's
essential edit, hear the result, and undo it while making tracker-only or
unsupported content unmistakable?

## Comments

The prototype should favor a thin, reviewable integration over production
completeness. It resolves the useful UI and code seam before Piano Roll Focus,
auxiliary polish, or broad tracker semantics are attempted.

### Native prototype debugging log — 2026-08-29 to 2026-08-30

The first human review failed. The owner reported that the pane was not
recognizable as a Piano Roll and supplied screenshots showing severe repaint
trails after vertical scrolling. The earlier statement that the prototype was
ready was too broad: the smoke test had only exercised native selection,
transpose, undo, and playback calls.

The repaint failure was reproduced with one Tracker scroll. Its cause is that
the fixed-looking pane was still inside `CViewPattern`'s `ScrollWindow` region,
so Tracker scrolling moved old Piano Roll pixels and only repainted the newly
exposed strip. The current debugging revision excludes the pane from that
pixel-scroll rectangle and invalidates the complete pane after every Tracker
scroll.

The follow-up review also exposed two distinct fixture/display problems. The
`test.mod` Pattern 2 playback is intentionally non-linear: its rows contain
pattern-loop, position-jump, and pattern-break commands, so the Tracker and
Piano Roll playheads jump together by design. All pitched events in that MOD
are on Channel 1; MOD LRRL setup therefore makes the rendered audio strongly
left-weighted. A command-line render with the repository's `openmpt123`
produced a 48 kHz float stereo WAV whose measured RMS values were 0.199738
(left) and 0.066580 (right), a 3.000:1 ratio. This confirms that the observed
jumps and left-heavy image are properties of the fixture, not a second
playback path in the Piano Roll.

The implementation now gives the projection its own child HWND with a private
bitmap paint pass and `WS_CLIPCHILDREN` on the Pattern view. The Tracker's
scroll and paint surface ends before the child pane, so refreshes cannot move
old Piano Roll pixels or expose channel columns underneath it. The pitch range
is computed from the whole active Pattern instead of the moving Tracker
cursor, preventing notes from jumping vertically during follow-song playback.
Selected notes use a filled light-gold body, a three-pixel halo, a contrasting
frame, and a dark grab handle rather than a one-pixel outline.

The visual review fixture is now `test/test.mod`, Pattern 2. It contains 16
pitched events spanning an octave, unlike the earlier `test.xm` Pattern 1 whose
single low note and many parameter commands could not establish Piano Roll
legibility. The revised pane removes the high-resolution width cap and adds a
visible piano keyboard, alternating black/white pitch lanes, a time ruler, and
wider minimum note blocks.

The final review used the following acceptance checks:

1. Scroll the Tracker up and down at least 20 times; the Piano Roll title,
   keyboard, grid, notes, legend, and state line must each remain single and
   intact with no trails.
2. In `test.mod` Pattern 2, the keyboard and multiple ascending/descending note
   blocks must be immediately recognizable as a time-horizontal Piano Roll.
3. Select a note from each surface and confirm the other surface follows.
4. Transpose one note, undo it through ordinary Pattern undo, and confirm both
   surfaces return to the same note.
5. Start normal Pattern playback and confirm one synchronized playhead; stop it
   and confirm the playhead clears without repaint residue.
6. Return to `test.mptm` and confirm PC/effect content remains visible as
   tracker-detail markers rather than fabricated pitched notes.

The changed Pattern translation units (`Draw_pat.cpp`, `View_pat.cpp`, and
their header) compile and link successfully with the standard x64 DEBUG
project target. The reviewed executable is
`openmpt-original_ref/bin/debug/vs2022-win10-static/amd64/OpenMPT.exe`, built at
2026-08-30 03:20:10 +08:00, 40,230,912 bytes, SHA-256
`FD87E183E86B6D5C12D13FF6D4E1DDBBD2A41CDECFF0E0BA65CC4CFF0645AC65`.
No Computer Use result is substituted for the owner's review.

- Build/run artifact: `openmpt-original_ref/bin/debug/vs2022-win10-static/amd64/OpenMPT.exe`.
- The prototype is an explicitly labelled, dependency-free GDI pane inside the
  existing `CViewPattern` Balanced layout. It reads and edits the active native
  `CPattern`; there is no second note store, persistence format, or parallel
  undo history.
- The Piano Roll uses conventional horizontal time and vertical pitch. Pitched
  notes and note termination events are projected from native Pattern commands.
  Volume commands, effects, and PC notes remain preserved and appear as orange
  tracker-detail markers with an explicit instruction to edit them in Tracker.
- Clicking or dragging a Piano Roll note updates the existing `PatternRect`
  selection and Tracker cursor. Dragging transposes the note; the `-1` and `+1`
  buttons exercise the same essential edit without requiring precise dragging.
- `Undo` uses the ordinary Pattern undo stack. `Hear (F7)` invokes normal
  Pattern playback, and both surfaces display the same playback position.
- The pane is intentionally available only when the Pattern view has enough
  room (at least 860 by 360 client pixels). It displays one current Pattern and
  does not attempt Piano Roll Focus, cross-Pattern Order editing, auxiliary
  automation lanes, note insertion, resizing, or production visual polish.

The superseded automated GUI smoke test used the built DEBUG executable without
saving the opened modules:

1. `test.mptm` showed preserved PC/tracker-only commands as explicit detail
   markers rather than fabricated Piano Roll notes.
2. In `test.xm`, clicking the projected note synchronized Tracker to row 0,
   channel 2, note `C-1`.
3. `+1` changed that same native event to `C#1` and dirtied the document;
   the pane's `Undo` restored `C-1` through ordinary Pattern undo. OpenMPT kept
   the document's modified marker after undo even though the event was restored;
   the on-disk test module was not saved.
4. `Hear (F7)` started normal Pattern playback and produced a synchronized
   playhead in Tracker and Piano Roll; playback was then stopped normally.

### Owner verdict — 2026-08-30

The owner rejected the first visual pass, then manually tested the rebuilt
executable after the child-pane and selection-style corrections. The owner
confirmed that the reported refresh flicker, apparent overlap with a
many-channel Tracker, and barely visible selected-note treatment were resolved.
This confirmation applies to the executable identity recorded above rather
than the stale 00:11 build used in the preceding failed report.

Together with the native interaction smoke evidence, the prototype answers its
question: a useful first slice can remain inside the existing Pattern Editor,
project the active native `CPattern` into a recognizable time-horizontal Piano
Roll, share `PatternRect` selection and playback state, mutate one native note,
and reuse ordinary Pattern undo without a parallel note store. Unsupported
tracker details must remain explicit markers, not invented Piano Roll notes.

The prototype does **not** establish production architecture or completeness.
It leaves Piano Roll Focus, note insertion and resizing, auxiliary lanes,
cross-Pattern / Order work, formal accessibility and polish, and broad tracker
semantics to later issues. The captured primary-source revision is commit
`18e3156fd0b5936e53a55bf68fcaa32a4ea70545` on branch
`prototype/synchronized-piano-roll-editing-slice`.
