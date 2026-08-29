# Inventory OpenMPT capabilities and architectural seams

Parent: ../map.md
Type: research
Status: resolved
Blocked by:

## Question

For the pinned Upstream Baseline, what user-visible OpenMPT capabilities exist, where do their query and mutation paths live, which already use document commands and undo, and which constraints prevent them from being safely shared with a Piano Roll or MCP adapter?

## Comments

### Implementation scope correction — 2026-08-29

This inventory remains useful evidence about the Starting Snapshot, but it is
not a migration checklist. The current map wraps only the existing operations
needed by the first vertical slice; other capability groups remain untouched
until an observed workflow requires them.

## Answer

### Baseline and method

This inventory is pinned to OpenMPT SVN `r25644`, represented by official Git-mirror commit [`0eafb124cfd15f94e32302734607e71f2c36c84f`](https://github.com/OpenMPT/openmpt/commit/0eafb124cfd15f94e32302734607e71f2c36c84f). All source links below are permanent links to that commit. The inventory follows user-visible editors and commands from their MFC handlers into `CModDoc`, `CSoundFile`, model containers, undo buffers, playback, and file code. It inventories capability groups rather than every menu alias or keyboard binding.

### Executive finding

OpenMPT has reusable model and document operations, but it does **not** have a uniform application-command layer.

`CModDoc` is the nearest existing application seam: it owns the canonical `CSoundFile`, the modified flag, view notifications, file lifecycle, playback handlers, and three undo managers. However, it also exposes a mutable `CSoundFile &`, and many views/controllers mutate that object directly before manually calling `SetModified()` and `UpdateAllViews()`. The relevant declarations are `CModDoc`, `GetSoundFile()`, `SetModified()`, the three undo accessors, and `UpdateAllViews()` in [`mptrack/Moddoc.h`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.h#L118-L312).

The existing “commands” are primarily MFC message-map handlers, not typed command objects with validation, atomic commit, rollback, stable errors, revision preconditions, and protocol-neutral results. The undo system is likewise three specialized snapshot histories—pattern/channel, sample, and instrument—not a document-wide transaction log ([`mptrack/Undo.h`, `CPatternUndo`, `CSampleUndo`, `CInstrumentUndo`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Undo.h#L18-L179)). Therefore existing methods are useful implementation material, but exposing them directly to a Piano Roll or MCP adapter would preserve UI coupling and inconsistent safety semantics.

### Existing architecture and state paths

```text
MFC command / editor event
        |
        +--> view or control owns the workflow and validation
        |      (`CViewPattern`, `COrderList`, `CCtrlSamples`,
        |       `CCtrlInstruments`, `CViewGlobals`, dialogs)
        |
        +--> sometimes calls a `CModDoc` operation
        |      (`InsertPattern`, `ReArrangeChannels`, playback, save...)
        |
        +--> often obtains mutable `CSoundFile &` and writes model data directly
               |
               +--> optionally prepares one specialized undo buffer
               +--> mutates patterns / sequence / channels / samples /
               |    instruments / plugins / song properties
               +--> manually sets document modified
               +--> manually broadcasts an `UpdateHint`

Audio callback / playback also reads and mutates `CSoundFile` play state,
with selected editor operations guarded by the global audio critical section.
```

Queries are similarly distributed. Views commonly read `CSoundFile` containers and public fields directly; `CModDoc` adds cross-model helpers such as used-sample/instrument checks, formatted names, edit position, and playback state. There is no immutable project snapshot DTO and no monotonic project revision in `CModDoc`; the only cross-cutting dirty state is the MFC modified flag plus an atomic “modified since autosave” boolean ([`CModDoc` state](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.h#L118-L175)).

### Capability inventory

| Capability group | User-visible capabilities | Current query path | Current mutation / operation path | Undo and sharing assessment |
|---|---|---|---|---|
| Project lifecycle and format compatibility | New/open/close; save, save-as, copy, template and compatibility export; load many tracker/module formats | `CModDoc` path and flags; `CSoundFile` format/type/specification and loader state | `CModDoc::OnOpenDocument()` calls `CSoundFile::Create()`; `CModDoc::SaveFile()` dispatches to MOD/S3M/XM/IT/MPT writers ([`Moddoc.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.cpp#L191-L402), [`Sndfile.cpp::Create`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/soundlib/Sndfile.cpp#L428-L513)) | File operations are not undo commands. They use paths, dialogs, warnings, settings and compatibility conversions. Keep protocol-neutral project serialization reusable, but initially keep arbitrary path selection and destructive conversion behind a separate, policy-checked operation boundary. |
| Song properties, metadata and comments | Module type; title/message; tempo, speed, rows per beat/measure; restart and global volumes; tempo mode, mix levels and compatibility flags | General/comment controls read `CSoundFile` fields directly | `CCtrlGeneral` and `CCtrlComments` write fields or `SongMessage` and call `SetModified()`; comments have no undo ([`Ctrl_gen.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Ctrl_gen.cpp), [`Ctrl_com.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Ctrl_com.cpp#L328-L346)) | Mostly no undo. Some format/type changes also transform data and present modal decisions. Queries are easy to share after snapshotting; mutations need new validated commands and document-wide undo coverage. |
| Order sequences and pattern lifecycle | Multiple sequences; insert/delete/move/copy orders; separators; restart; create, duplicate, merge, split, resize, name and remove patterns; render selected orders | `COrderList::Order()` returns the current mutable `ModSequence`; sequence and pattern containers live in `CSoundFile` ([`Ctrl_seq.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Ctrl_seq.cpp#L109-L131)) | `COrderList` directly inserts/removes/assigns order entries and manually updates the document; pattern lifecycle is split between controllers and `CModDoc::InsertPattern/RemovePattern/MoveOrder` ([`Ctrl_seq.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Ctrl_seq.cpp#L1279-L1498), [`Modedit.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Modedit.cpp#L678-L969)) | Pattern deletion and many pattern-data transforms prepare pattern undo. Order insertion/deletion/move/restart and pattern insertion are not covered by a sequence/document undo history. This prevents an atomic “create pattern and insert it in sequence” capability without a new transaction. |
| Tracker Pattern editing | Read/write note, instrument, volume and effect cells; selection, clipboard modes, clear, row insert/delete, transpose, interpolate, amplify, grow/shrink, find/replace, split; live/MIDI entry and plugin-control events | `CViewPattern` reads patterns and returns direct `ModCommand` references (`GetModCommand`) from `CSoundFile::Patterns` ([`View_pat.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_pat.cpp#L3898-L3907)) | `CViewPattern` and helpers validate cursor/selection state, call `CPatternUndo::PrepareUndo()`, write `ModCommand` cells, then call their local `SetModified()` wrapper and broadcast hints ([`View_pat.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_pat.cpp#L427-L694)) | Best existing basis for the Piano Roll and first shared edit commands. Undo can link multiple pattern snapshots, but there is no protocol-neutral edit request, no expected revision, and editor selection/cursor/record-mode assumptions must be removed from the core command. |
| Channels and mixer defaults | Add/remove/reorder/duplicate channels; name, color, mute/solo/pending mute, surround, pan, volume and plugin routing | Pattern/general views read `ChnSettings`, runtime channel flags and playback state | `CModDoc::ReArrangeChannels()` snapshots all patterns plus channel information, enters `CriticalSection`, reallocates/reorders pattern cells and settings; smaller channel edits are often direct view writes ([`Modedit.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Modedit.cpp#L46-L299), [`View_gen.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_gen.cpp#L683-L705)) | Persistent channel settings can use the pattern undo buffer’s channel snapshot. Mute/solo may be runtime-only or document-modifying depending on format/settings. A shared API must distinguish persisted channel configuration from transient playback state. |
| Samples | Create/duplicate/remove; import/export; waveform selection/drawing; loops/cues; normalize, amplify, resample, pitch/time stretch, reverse, silence, DC removal, crossfade, mono/stereo conversion; external sample paths and OPL data | Sample controls/views directly read `ModSample`, sample data, names and paths from `CSoundFile` | `CCtrlSamples` and `CViewSample` prepare `CSampleUndo`, invoke `CSoundFile` sample algorithms or edit buffers, then set modified and send `SampleHint`; load/save paths go through dialogs and format writers ([`Ctrl_smp.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Ctrl_smp.cpp#L932-L990), [`View_smp.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_smp.cpp#L195-L206)) | Broad per-sample undo exists, including range-aware and replace snapshots. Slot creation/removal, related instrument mapping, external-file flags and batch operations are not one atomic history. DSP can be shared later as cancellable commands with memory/time limits; filesystem import/export needs explicit path policy. |
| Instruments | Create/duplicate/remove; import/export; name and playback properties; sample/note map; NNA/DCT/DNA; MIDI/plugin assignment; volume/pan/pitch/filter envelopes; tuning | Instrument controls/views directly dereference `CSoundFile::Instruments[]`, sample maps and envelopes | `CCtrlInstruments`, `CNoteMapWnd` and `CViewInstrument` prepare `CInstrumentUndo`, edit `ModInstrument`, set modified and emit `InstrumentHint`; some actions also prepare sample undo ([`Ctrl_ins.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Ctrl_ins.cpp#L1173-L1186), [`View_ins.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_ins.cpp#L194-L207)) | Strong per-instrument snapshot undo, but operations spanning instrument and sample slots create separate histories. Reusable property/map/envelope edits need one combined transaction and format-capability validation. |
| Plugins, routing and automation | Select/load/remove/clone/move plugins; route outputs; mix/bypass/dry-wet; presets/programs/parameters; native editor; MIDI mapping; record parameter changes into patterns | `CViewGlobals` and plugin editors read/write `m_MixPlugins[]` and live `IMixPlugin`; pattern automation is stored as PC events | Plugin-slot workflows live in `CViewGlobals`, `CModDoc::RemovePlugin/ClonePlugin`, plugin manager and plugin editor objects; automation recording writes pattern cells ([`View_gen.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_gen.cpp#L1059-L1173), [`Modedit.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Modedit.cpp#L590-L675), [`View_pat.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_pat.cpp#L3753-L3837)) | Pattern automation events use pattern undo; plugin graph/state operations have no general undo and may execute third-party code, open native windows, scan files or block. Keep plugin lifecycle and arbitrary parameter access UI-only initially; later expose a narrow allowlisted, timeout-aware operational surface. |
| Playback, audition and transport | Play/pause/stop; from start/order/row; pattern loop; queued transitions; follow song; metronome; audition note/sample/instrument; mute/solo; position/time/VU notifications | `CSoundFile` play state plus `CMainFrame` device/notification state; document and views query both | Document handlers delegate song transport to `CMainFrame::PlayMod/PauseMod/StopMod`; audio callback calls `CSoundFile::Read`; preview uses separate `PlaySoundFile` paths ([`Moddoc.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.cpp#L2122-L2245), [`Mainfrm.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Mainfrm.cpp#L1506-L1778)) | Operational, not undoable. Existing code is coupled to the singleton main frame, sound device and live play state. A shared layer needs serialized UI-thread control, explicit transient state, cancellation, and events; it must never call editor code on the realtime callback. |
| Render, conversion, cleanup and append | Render song/range/stems to audio or sample; MIDI/OPL export; append module; cleanup/remove unused data; format conversion | Dialog workflows derive settings and inspect the whole `CSoundFile` | `CModDoc::OnFileWaveConvert()` pauses playback, shows dialogs, mutates render/mute state, runs render passes and writes files; cleanup/append/conversion touch several model domains ([`Moddoc.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.cpp#L1678-L2027), [`CleanupSong.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/CleanupSong.cpp), [`AppendModule.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/AppendModule.cpp)) | Some cleanup substeps prepare several specialized undo snapshots, but there is no atomic whole-document rollback. Rendering is long-running and stateful. Expose preview rendering only after extracting dialog-free settings, snapshot isolation, progress/cancellation, output limits and safe destination policy. |
| Application settings and integrations | Audio/MIDI device settings; keyboard map; colors/layout; autosave; plugin paths/scanning; update/download and shell/file associations | Global `TrackerSettings`, app/main-frame state and configuration files | Settings dialogs and application services, outside the module document | These are not composition-project capabilities. Keep them out of the initial Application Capability layer and MCP surface; expose only narrowly required health/status queries. |

### Undo audit

| Domain | Existing mechanism | What it reliably covers | Important gaps |
|---|---|---|---|
| Pattern data and channel headers | `CPatternUndo` stores pattern rectangles, deleted patterns and optional channel settings; linked entries can form one UI undo step ([implementation](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Undo.cpp#L32-L275)) | Cell edits, selection transforms, pattern resizing/removal, many all-pattern channel operations | Sequence positions, song properties, samples, instruments and plugins are not in the same transaction. “Linked” means linked pattern snapshots, not arbitrary document changes. |
| Sample | `CSampleUndo`, per sample, with specialized update/delete/insert/replace/reversible operation types ([declarations](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Undo.h#L69-L139)) | Most waveform and sample-property edits | Slot topology, instrument references, external files and other domain edits are outside the same history. |
| Instrument | `CInstrumentUndo`, per instrument or envelope ([declarations](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Undo.h#L142-L179)) | Instrument properties, maps and envelopes | Associated sample changes, slot topology, plugins and pattern references are separate. |
| Sequence, song metadata, plugin graph, file operations and transport | No corresponding general undo manager | Selected callers may create ad-hoc pattern/sample/instrument snapshots | No complete rollback or redo contract. Many operations only mark dirty and notify views. |
| Cross-domain edit | None | Nothing guarantees atomicity across the three histories | A failure after the first mutation can leave partial work; no single audit record, revision increment, inverse or redo item exists. |

The undo APIs are “prepare before mutating” snapshot calls. They do not execute the mutation, validate it, or roll it back on failure. This is why “already has undo” must not be interpreted as “already is a safe shared command.”

### Constraints that block direct reuse

1. **Mutable model escape hatch.** Any caller can obtain non-const `CSoundFile &` from `CModDoc`, bypassing validation, dirty tracking, undo and notifications.
2. **UI-owned workflows.** Important validation and orchestration live in views, controls and modal dialogs. Many document methods call `Reporting`, `CMainFrame`, `theApp`, `FileDialog`, wait-cursor functions or native plugin editors, so they are not headless or protocol-neutral.
3. **No project revision or stale-write check.** `SetModified()` records dirtiness, not a monotonic content identity. Neither the Piano Roll nor MCP can safely submit a proposal against an expected revision without new infrastructure.
4. **Fragmented undo and no atomic multi-domain transaction.** Three histories cannot represent one proposal that changes sequence, patterns, instruments and samples together.
5. **Manual notification discipline.** Callers choose `UpdateHint` and sender suppression after mutation. A missed or premature hint can desynchronize views. `CModDoc` explicitly distinguishes a GUI-thread `UpdateAllViews` overload from a non-GUI-thread posting overload ([declarations](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.h#L308-L313), [implementation](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.cpp#L1642-L1675)).
6. **Audio concurrency is not an application transaction.** `CriticalSection` protects selected edits against concurrent audio access, but it neither marshals work to the UI thread nor provides validation, rollback, cancellation or bounded latency. Long work must not hold it or execute on the audio callback.
7. **Format-dependent semantics.** Legal channels, rows, instruments, effects, note ranges, plugin support and persisted flags depend on the module type’s specifications. Shared commands must validate against the current format and report loss/conversion explicitly.
8. **External side effects.** Sample/instrument/project I/O, external samples, render destinations, plugin loading/scanning and native plugin code cross filesystem/process boundaries. They require path allowlists, resource limits, cancellation and separate approval policy.
9. **Transient and persisted state are mixed.** Playback position, queued transitions, previews and some mute flags coexist with editable project state. Score Context and Change Proposals must exclude or label transient state rather than treating every visible value as project content.
10. **Unstable raw representation.** Pointers, MFC objects, indices whose meaning changes after reorder, and live plugin instances are not suitable API values. Shared capabilities need stable IDs/scopes, plain data transfer objects and explicit invalidation rules.

### Reusable seams and initial boundary recommendation

This section is a downstream recommendation inferred from the source inventory, not an upstream OpenMPT contract.

**Reuse behind adapters:**

- **Canonical Tracker model and playback reuse the original OpenMPT implementation:** `CSoundFile` and its Pattern, Order, Sample, Instrument, Plugin, and playback containers remain authoritative rather than being reimplemented for the Piano Roll or AI layer.
- **Format validation, loaders/writers, serializers, and existing Pattern/Sample/Instrument edit algorithms reuse the original OpenMPT implementation** behind typed adapters.
- **Domain-local undo and redo reuse the original OpenMPT implementation:** the existing Pattern, Sample, and Instrument undo buffers initially back narrowly scoped commands that touch exactly one supported domain, provided the adapter owns prepare/mutate/failure cleanup/notification. A new transaction coordinator is still required for cross-domain atomicity.
- **Offline playback rendering and stream encoding reuse the original OpenMPT implementation** once the existing dialog-driven workflow is isolated behind a cancellable, snapshot-bound adapter.
- `UpdateHint` can remain the internal bridge to legacy views, emitted once after a successful shared command.

**Safe first shared queries:**

- project identity, format/specification and dirty/revision metadata;
- sequences and pattern metadata;
- bounded Tracker Pattern ranges as plain semantic data;
- channel, sample and instrument metadata without raw pointers or sample bytes by default;
- playback/transport status as explicitly transient state;
- logs and capability availability.

All queries should execute against one UI-thread-marshalled snapshot and return the revision captured with that snapshot.

**Safe first shared edits:**

- bounded Pattern cell/range replacement with format validation and pattern undo;
- a small set of explicit pattern transforms whose current implementations already use one pattern undo step;
- persistent channel property edits only after transient mute/solo is separated;
- narrowly scoped sample or instrument property edits after each is wrapped as one typed transaction.

Every edit should accept `expectedRevision`, prevalidate the complete request, prepare undo, mutate once on the owning thread, set modified, increment the revision, emit legacy hints plus one capability event, and return a stable result. A failed command must leave revision and project state unchanged.

**Keep UI-only initially:**

- sequence topology and compound pattern lifecycle until sequence/document undo exists;
- module-type conversion, cleanup, append and other broad cross-domain transforms;
- plugin discovery/loading/editors and unrestricted plugin parameter access;
- arbitrary filesystem import/export and external-sample relinking;
- render/export until it is isolated, cancellable and policy-bounded;
- global application/device/settings administration.

### Decision supplied to issue 05

The shared Application Capability layer should be a new, narrow façade **above** `CModDoc`/`CSoundFile`, not a direct export of MFC command IDs, view handlers or the mutable `CSoundFile` reference. Its first implementation slice should establish the missing invariants—owning-thread execution, immutable queries, project revision, expected-revision edits, typed validation/errors, atomic undo adapter and post-commit events—using bounded Pattern queries and edits as the proving ground. Other capability groups can migrate only when they satisfy the same contract.
