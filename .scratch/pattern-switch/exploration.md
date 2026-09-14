# Exploration: Pattern switching via MCP (`get_pattern_order`, `switch_pattern`)

Read-only investigation of `E:\My AI-s Memory\Code\OpenMPT-for-AI` (branch `main`, HEAD
`a83f7bc2718cb7cd8730e29d4d630df2866f0ce2`, "钢琴卷帘逻辑优化", 2026-09-10).
Goal: map the existing Pattern MCP seams that a planned `get_pattern_order` /
`switch_pattern` feature must build on, and identify what is missing.

No product or test file was modified. The only artifact is this report.

---

## 1. Bottom line

1. **The five-tool Pattern MCP surface exists end to end** and is the anchor for the
   new work: `AI::PatternCapability` (C++) → `Panel::Dispatch` (owning-thread facade)
   → `Broker` named pipe → `sidecar/openmpt_mcp.py` stdio MCP tools. See §2.
2. **`get_pattern_order` and `switch_pattern` do not exist anywhere in the tree.**
   `grep -rn "get_pattern_order\|switch_pattern"` over `*.py, *.cpp, *.h, *.md, *.yaml`
   (excluding `.git/`, build outputs) returns no matches. The Sidecar tool table
   (`sidecar/openmpt_mcp.py:29-62`), the C++ tool allowlist
   (`openmpt-original_ref/mptrack/AIPattern.cpp:199-200`), and the skill docs still
   declare exactly five tools.
3. **The current capability cannot switch patterns in place.** `m_pattern` is
   `const PATTERNINDEX` (`AIPattern.h:42`) and `Dispatch` rejects any request naming
   another pattern with `boundPatternViolation` (`AIPattern.cpp:202-203`). Switching
   must either replace the `unique_ptr<PatternCapability>` held by `Panel`
   (`AIService.cpp:379`, created at `AIService.cpp:633`) or add a rebind API.
4. **The existing approval machinery is expansion-specific.** `m_pending` stores the
   `replace_pattern_segment` arguments (`AIPattern.cpp:341-345`) and is replayed by
   `ResolveExpansion(bool)` through `Replace(args, true)` (`AIPattern.cpp:363-374`);
   `ExpansionRange()` decodes those exact keys (`AIPattern.cpp:375-381`). A switch
   approval needs its own pending representation and a non-expansion resolution path
   in `ReviewPanel::OnCommand` (`AIService.cpp:958-961`) and `ReviewPanel::OnPaint`
   (`AIService.cpp:903-909`).
5. **There is no write occupancy on a plain read path today except the implicit
   short occupancy of `get_pattern_context`.** A session-less
   `get_pattern_context` sets `AIOccupied(true)` and releases it in `Call`'s
   `EndCall` destructor (`AIPattern.cpp:210-216`, `AIPattern.cpp:180-191`).
   `Panel::Dispatch` only materialises a capability for `get_pattern_context`
   (`AIService.cpp:628-634`), so `get_pattern_order` needs a new read-only branch
   (or an order-only capability) that never calls `SetAIOccupied`.
6. **"Dirty" has no explicit gate.** `grep IsModified` in `AIPattern.*`/`AIService.*`
   returns nothing. The available rejection tools are `approvalPending`
   (`AIPattern.cpp:204`), `busy` while retained/proposal exists
   (`AIPattern.cpp:212`), `occupancyLost`/`candidateExists` for un-handed edits
   (`AIPattern.cpp:207`, `231`), `stale` for dependency drift
   (`AIPattern.cpp:208`), and `Review()`'s `status: "current"/"stale"`
   (`AIPattern.cpp:388`). Which of these means "dirty" must be decided; see §15.
7. **Patterns-display sync without sequence/order/play change is directly available**
   via `CViewPattern::SetCurrentPattern` + `SetCurSel(cursor)`/`SetSelToCursor`
   (`View_pat.cpp:214-258`, `View_pat.h:260-263`), demonstrated by the review-evidence
   click path (`AIService.cpp:972-974`). Do **not** go through
   `COrderList::SetCurSel` (`Ctrl_seq.cpp:343-424`, changes play position) or
   `CCtrlPatterns::OnActivatePage` (`Ctrl_pat.cpp:592-625`, may reselect sequence/order).
8. **Settings, atomic apply/Undo, sidecar pinning, and test seams are all reusable**
   as-is; the concrete line-level maps are in §4–§11.

---

## 2. Exact public C++ capability

### 2.1 `AI::PatternCapability` (the only Pattern-mode facade)

Declared in `openmpt-original_ref/mptrack/AIPattern.h`, implemented in
`openmpt-original_ref/mptrack/AIPattern.cpp`.

| Public member | Location | Notes |
| --- | --- | --- |
| `PatternCapability(CModDoc&, PATTERNINDEX, std::optional<PatternRect>)` | `AIPattern.h:22` | binding is fixed at construction |
| `Json Call(tool, args)` | `AIPattern.h:26` | UI-thread guard; `ending_reminder` appended once |
| `Json Apply(bool whole = true, bool simulateFailure = false)` | `AIPattern.h:27` | atomic commit; test-only failure injection |
| `Json Reject()` | `AIPattern.h:28` | discards proposal |
| `Json Review() const` | `AIPattern.h:29` | diff + `status` current/stale |
| `Json ExpansionRange() const` | `AIPattern.h:30` | pending-range UI data |
| `Json ResolveExpansion(bool approve)` | `AIPattern.h:31` | resumes a pending expansion |
| `void ForceRelease()` | `AIPattern.h:32` | drops occupancy/candidate/pending |
| `void Tick(now)` | `AIPattern.h:33` | expiry only while retained and not pending |
| `Occupied() / PendingExpansion() / HasProposal() / Pattern()` | `AIPattern.h:34-37` | `Pattern()` is read-only getter |
| `void Configure(unsigned seconds, bool alwaysApprove)` | `AIPattern.h:38` | only two settings today |

Private state that constrains a switch implementation:
`const PATTERNINDEX m_pattern` (`AIPattern.h:42`), `m_baseline`/`m_candidate`
(`:45`), `m_signature`/`m_token` (`:46`), `std::optional<Json> m_pending` (`:47`),
`m_envelope` (`:44`), `m_alwaysApprove` (`:51`), `m_ownsOccupancy` (`:52`),
`m_deadline` (`:53`).

Tool dispatch inside the class:

- `Call` enforces the owning thread and non-reentrancy, then delegates to `Dispatch`
  and appends `ending_reminder` (`AIPattern.cpp:175-191`).
- `Dispatch` tool allowlist: exactly the five names (`AIPattern.cpp:199-200`).
- Bound-pattern guard: an explicit `"pattern"` argument must equal `m_pattern`
  (`AIPattern.cpp:202-203`).
- Pending guard: any call while an expansion waits returns `approvalPending`
  (`AIPattern.cpp:204`).
- Token/session check and staleness (`AIPattern.cpp:205-209`).
- Short-occupancy vs retained read and `occupy=true` promotion
  (`AIPattern.cpp:210-227`).
- `abort_session` / `release_occupancy` (`AIPattern.cpp:229-233`; edited-candidate
  release fails with `candidateExists`).
- `replace_pattern_segment` → `Replace` (`AIPattern.cpp:235`).
- `handoff_for_review` freezes `m_proposal` and calls `ForceRelease`
  (`AIPattern.cpp:236-241`).

Other key methods: `Configure` (`AIPattern.cpp:66-70`), `ForceRelease`
(`:71-78`, clears `m_pending`; only clears baseline/candidate when no proposal),
`Tick` (`:80-83`), `Capture` (`:84-101`, full-pattern baseline, whole-pattern
envelope when no selection at `:93`, GUID session token at `:95-100`),
`Signature` (`:103-173`, pattern + samples + instruments + tempo/swing/format
dependency record), `Replace` (`:294-361`), `ResolveExpansion` (`:363-374`),
`ExpansionRange` (`:375-381`), `Review` (`:383-389`), `Reject` (`:391-397`),
`Apply` (`:399-425`).

### 2.2 Panel facade and dispatch

`openmpt-original_ref/mptrack/AIService.cpp`:

- `Panel` owns `std::unique_ptr<PatternCapability> capability` (`:379`),
  `capabilityConnection` (`:391`), `document` (`:381`) and `pending` (`:382`).
- `Panel::Dispatch` (UI/owning thread): instance identity `:608`, document lookup by
  `CModDoc::AIIdentity()` `:610-612`, attach handling `:613-620`, per-document busy
  `:625`, per-connection ownership `:626-627`, session-less context binding from the
  Patterns view `:628-634`, `Configure` `:636`, `Call` `:637`,
  `capabilityConnection` capture `:638-640`.
- `CViewPattern *PatternView(CModDoc&)` helper: `AIService.cpp:332-337`.
- `Panel` controls enum (settings/approval/review IDs): `AIService.cpp:339`.
- `DocumentClosed` releases per-document capability/pending and refreshes review
  (`AIService.cpp:1179-1192`); called from `CModDoc::~CModDoc`
  (`Moddoc.cpp:158`).

Transport: `Broker` (`AIService.cpp:106`) creates a current-user named pipe
(`AIService.cpp:245-330`), validates envelopes, queues on the UI thread; a
`direct:true` call is rejected on the broker thread with `owningThreadRequired`
(`AIService.cpp:207-209`; `AIService.cpp:408-419` direct probe).

### 2.3 Sidecar public MCP API

`sidecar/openmpt_mcp.py`:

- `PROTOCOL = "2025-11-25"`, `MAX_MESSAGE = 4 MiB` (`:9-10`).
- `TOOLS` list of five schemas (`:29-62`); `SESSION` token schema (`:22`); `RANGE`
  (`:26-27`).
- `ENDING_TOOLS = {handoff_for_review, abort_session, release_occupancy}` (`:13`).
- `Sidecar.handle` `initialize` / `ping` / `tools/list` / `tools/call` (`:207-224`);
  unknown tool → JSON-RPC `-32602` (`:218-219`).
- `Sidecar.call` performs attach if needed, sends the `call` envelope, maps
  transport disconnects to `instanceGone`, malformed results to `schemaFailure`
  (`:178-206`).
- Result shape: `{isError, structuredContent, content[text]}` (`tool_result`, `:64-66`).

Protocol envelope (README `sidecar/README.md` "App envelope"; code):
`{"version":1,"operation":"attach","instance":...,"document":...}` then
`{"version":1,"operation":"call","instance":...,"document":...,"tool":...,"arguments":...}`,
4-byte little-endian length framing, 4 MiB max.

**Implication:** a new tool requires changing at least `TOOLS`,
`openmpt_mcp.py:218-219`, `AIPattern.cpp:199-200`, and the exactly-five-tools tests
(§10).

---

## 3. Bound session ownership

App side ("who owns the write lease"):

- Document-wide occupancy flag: `CModDoc::m_aiOccupied` (`Moddoc.h:134,169-170`),
  acquired only by `PatternCapability` (`AIPattern.cpp:213-215`), released through
  `m_ownsOccupancy` (`AIPattern.cpp:74-76`).
- Pattern binding: `const PATTERNINDEX m_pattern` (`AIPattern.h:42`); baseline and
  candidate are full copies captured in `Capture` (`AIPattern.cpp:84-101`).
- Session token: `m_token` GUID, checked against the wire `session` argument
  (`AIPattern.cpp:207`); never exposed in `Review`/`Apply`.
- Connection ownership: `Panel::capabilityConnection` (`AIService.cpp:391`),
  enforced as `busy` for other connections (`AIService.cpp:626-627`) and set when an
  occupied result carries a session (`AIService.cpp:638-640`). Ownership is per
  exact broker connection; seats/stealing are not possible.
- Manual view navigation never edits `capability`: the only place the view pattern is
  read is the session-less context binding (`AIService.cpp:628-633`), and the facade
  rejects another pattern afterwards (`AIPattern.cpp:202-203`). This matches
  ticket 35 AC1 ("Agent session remains pinned to its original Pattern") in
  `.scratch/ai-collaborative-openmpt/issues/35-fix-retained-occupancy-owner-ui-usability.md`.

Sidecar side:

- `Sidecar.target_identity` tuple `(pipe, instance, document, generation)` captured in
  `refresh_target` (`openmpt_mcp.py:123-125`); `retained_session` boolean
  (`:90`, `:137-143`); target switch blocked while retained unless the tool is an
  ending tool (`:126-133`); `attachment_error` latch (`:185-205`). Verified by
  `sidecar/test_sidecar.py:149-171` (`test_target_switch_waits_for_retained_work_to_end`)
  and `:173-186` (re-publish clears latch).
- The Sidecar does not store the token value; it only remembers that a session is
  retained. Token capture is client-side.

---

## 4. Scope approval waits, cancellation, timeouts

Observed flow (expansion today; the pattern a switch approval would follow):

1. `Replace` detects an out-of-envelope write; unless `approved` or
   `m_alwaysApprove`, it stores the request in `m_pending` and returns
   `{ok:true, pending_approval:true}` (`AIPattern.cpp:341-345`).
2. `Panel::OnTimer` sees `pending_approval` and holds the broker `Request` instead of
   completing it (`AIService.cpp:779`); the waiting broker thread stays in
   `Request::ready.wait_for` (`AIService.cpp:79-91`, `:214-220`).
3. `ReviewPanel::OnCommand` on Approve/Decline calls
   `pending->Complete(capability->ResolveExpansion(approved))` (`AIService.cpp:958-961`).
   `ResolveExpansion` checks the owning thread and retained state, clears
   `m_pending`, re-checks `Signature()`, refreshes `m_deadline`, and either replays
   `Replace(args, true)` or returns `rangeRejected` (`AIPattern.cpp:363-374`).
4. UI surfaces: `UpdateControls` enables Approve/Decline only while `pending` exists
   (`AIService.cpp:868-869`); `OnPaint` renders "approval waiting (timer paused)" and
   `ExpansionRange()` (`AIService.cpp:903-909`).
5. Preference: `Panel` reads `AI/MCP/AlwaysApprove` (`AIService.cpp:438`), writes it
   (`:678`), and pushes `Configure(seconds, always)` into the capability on every
   dispatch (`:636`) and settings change (`:680`). `Configure` stores
   `m_alwaysApprove` (`AIPattern.cpp:66-70`); `Replace` consults it at `:341`.

Timeout / cancellation:

- Default 300 s, clamped 1–3600 s (`AIService.cpp:436`, `AIPattern.cpp:67-68`);
  `m_deadline` is set on occupied read (`AIPattern.cpp:224`), edit (`:359`),
  and resolution (`:369`).
- `Tick` expires only `m_retained && !m_pending && now >= m_deadline`
  (`AIPattern.cpp:80-83`), so an approval wait pauses the timer; `Panel::OnTimer`
  calls `Tick` every 100 ms (`AIService.cpp:449`, `:743`). Test:
  `mpt_tests_ai_pattern.cpp:179-181`.
- Cancellation paths that complete `Panel::pending` with `occupancyLost`:
  `ReleaseNow` (`AIService.cpp:513-517`), session expiry (`:745`), disconnect of the
  owning connection (`:595-602`), document close (`:1190`). `ForceRelease` drops
  `m_pending` (`AIPattern.cpp:77`). Tests: `mpt_tests_ai_pattern.cpp:196-201`,
  `:163-165`.

**Gap:** `m_pending` is an `optional<Json>` of replace arguments. There is no pending
kind or generic approval envelope, so a switch-approval cannot simply reuse
`ResolveExpansion`.

---

## 5. Atomic review apply and native Undo

`PatternCapability::Apply` (`AIPattern.cpp:399-425`):

- Refuses wrong thread, partial acceptance (`unsupported`), no proposal, occupied
  document (`busy`), dependency drift (`stale`), all-cell revalidation failures.
- `simulateFailure` returns `commitFailed` before touching anything (`:411`) — the
  deterministic failure seam used by tests.
- `emptyProposal` guard (`:412`).
- Single native Undo preparation for the whole pattern:
  `GetPatternUndo().PrepareUndo(m_pattern, 0, 0, m_channels, m_rows, "Apply AI proposal")`
  (`:415`); copy under `CriticalSection` (`:417-420`); `SetModified()` (`:421`,
  increments `m_aiRevision` at `Moddoc.cpp:169-171`); `UpdateAllViews` with
  `PatternHint` (`:422`); then clear `m_proposal` and release (`:423-424`).
- Every failure path returns before `m_proposal = false`, so the frozen proposal (and
  its diff via `Review()`) survives for retry.

Undo system: `CPatternUndo` (`mptrack/Undo.h:28-79`), accessed through
`CModDoc::GetPatternUndo()` (`Moddoc.h:228`). While AI-occupied, `Undo()`/`Redo()`
return `PATTERNINDEX_INVALID` (`Undo.cpp:164-176`). Tests prove: handoff changes no
revision (`mpt_tests_ai_pattern.cpp:103-113`), failed Apply leaves revision/Undo/cells
untouched and proposal intact (`:119-127`), Apply advances revision exactly once
(`:129`), one native Undo restores all voices and leaves `!CanUndo()`
(`:132-133`), stale proposal retained (`:148-152`), Reject preserves revision and
Undo history (`:137-142`).

---

## 6. Settings and UI

Current settings (section `AI/MCP`, read lines `AIService.cpp:436-439`, write lines
`:677-679`):

| Key | Default | Read | Write |
| --- | --- | --- | --- |
| `Enabled` | `true` | `:439` | `:677` |
| `AlwaysApprove` | `false` | `:438` | `:678` |
| `TimeoutSeconds` | `300` | `:436` | `:679` |

Persistence uses `theApp.GetSettings().Read<T>/Write<T>(section, key, default)`
(`mptrack/Settings.h:636-640`); the `AI/MCP` section is the established namespace.

UI structure:

- Upper connection/settings row (Panel ctor `AIService.cpp:423-451`): `Enable MCP`
  checkbox `:433`, `Always approve range expansion` `:434`, timeout edit `:435`,
  `Save settings (seconds)` `:444`, `Connect active doc to Codex` `:445`, identity
  text `:446`; layout `:485-511`.
- Lower review pane (`ReviewPanel` ctor `:791-806`): `RELEASE AI NOW` `:798`,
  `Approve expansion` `:799`, `Decline expansion` `:800`, `Apply whole proposal`
  `:801`, `Reject whole proposal` `:802`, diff list `:803`, painted evidence
  `:914-949`.
- Setting change handler: `AIService.cpp:674-681` (`SettingsSave|Enable|AlwaysApprove`
  → persist, start/stop broker, `Configure`, `RefreshReview`).
- Review actions: `AIService.cpp:951-977`.
- Approve/Decline enabled only while a pending request exists (`:865-871`).

**Gap:** the `AlwaysApprove` toggle does not resolve an already-pending request; it
only affects later requests (ticket 32 confirms "changing the preference takes effect
for subsequent expansion requests"). The new requirement ("enabling handles pending
requests") has no precedent and needs a new branch in `Panel::OnCommand`.
New checkboxes also need new IDs in the `Control` enum (`AIService.cpp:339`) and
layout/reflow updates (`:485-511`).

User-facing docs that must change: `USER_GUIDE.md:147-171` (AI/MCP page), `:192-198`
(five capabilities), `:235-238` (currently instructs the owner to end the session
before switching Pattern).

---

## 7. Patterns navigation and display sync

Data/navigation APIs:

- Current view binding: `CViewPattern::GetCurrentPattern()` (`View_pat.h:227`),
  `SetCurrentPattern(pat, row)` (`View_pat.h:265`, impl `View_pat.cpp:214-258`).
  `SetCurrentPattern` rejects `PATTERNINDEX_SKIP`/`PATTERNINDEX_INVALID`, changes
  only `m_nPattern`, sends `CTRLMSG_PATTERNCHANGED` (`View_pat.cpp:255`), and does not
  touch `m_nOrder`/play state.
- Order/sequence: `CViewPattern::Order()` returns `CSoundFile::Order()`
  (`View_pat.cpp:149-150`); `GetCurrentOrder()/SetCurrentOrder()` (`View_pat.h:230-231`,
  `View_pat.cpp:273-277`). `SetCurrentOrder` changes the order list selection.
- Selection: `SetCurSel(...)` / `SetSelToCursor()` (`View_pat.h:260-263`);
  `AISelection()` returns `nullopt` when selection is collapsed to one cell
  (`View_pat.h:226`). Clearing selection therefore = `SetCurSel(cursor)` after
  `SetCursorPosition(cursor)` (the review-evidence example at `AIService.cpp:972-974`).
- Tab/page plumbing: `Page::Patterns`/`Page::AI` (`Globals.h:117-130`), AI page tab ID
  `AI::PanelPageId` (`AIService.h:10`), page activation and panel attach
  (`Globals.cpp:349-470`), `CModDoc::ActivateView` (`Moddoc.cpp:1616-1637`),
  `CModDoc::ViewPattern` (`Moddoc.cpp:804-807`), `CViewPattern` creation via
  `IDD_CONTROL_PATTERNS` (`Ctrl_pat.cpp`, `ModCtrl_dlg`).

What changes sequence/order/play position (avoid for switch):

- `COrderList::SetCurSel(sel, setPlayPos=true, ...)` takes the "follow song" branch
  and calls `sndFile.SetCurrentOrder(m_nScrollPos)` and `SetElapsedTime`
  (`Ctrl_seq.cpp:384-416`); `SetCurrentOrder`/play state lives in
  `CSoundFile::m_PlayState` (getter `Sndfile.h:713`).
- `CCtrlPatterns::OnActivatePage` for a pattern item searches all sequences and calls
  `m_OrderList.SelectSequence(seq)` + `SetCurSel(ord, true)` (`Ctrl_pat.cpp:592-625`).
  Re-activating the Patterns tab with a pattern argument can therefore move the
  sequence/order selection. Toggling only `SetCurrentPattern` on an existing view
  avoids this.
- `COrderList::UpdateView` does not respond to plain `PatternHint`
  (`Ctrl_seq.cpp:682-693`), so the display update triggered by
  `CTRLMSG_PATTERNCHANGED` (`Ctrl_pat.cpp:418-420`) does not reorder anything.

Order-list data model for `get_pattern_order`:

- Current sequence: `ModSequenceSet Order` (`Sndfile.h:520`), `Order()` returns the
  working `ModSequence` (`ModSequence.h:156-157`), current index
  `GetCurrentSequenceIndex()` (`:163`), count `GetNumSequences()` (`:162`).
- Entries are raw `PATTERNINDEX` values in a `std::vector` (`ModSequence.h:26`);
  `"---"` = `PATTERNINDEX_INVALID` and `"+++"` = `PATTERNINDEX_SKIP`
  (`Snd_defs.h:30-31`).
- Validity: `Patterns.IsValidPat(i)` (`PatternContainer.h:77`); container size and
  highest valid pattern: `Size()` (`:68`), `GetNumPatterns()` (`patternContainer.cpp:145-155`).
- Sequence-level validity: `ModSequence::IsValidPat(ord)` (`ModSequence.cpp:254-259`),
  `FindOrder` (`ModSequence.cpp:270+`).
- No existing helper enumerates "unreferenced valid patterns"; a loop over
  `[0, Patterns.Size())` filtered by `IsValidPat` and absence from the sequence is
  required. The order-list UI already handles skip/stop drawing
  (`Ctrl_seq.cpp:748-751`, `:881-887`).

---

## 8. Sidecar document pinning and token capture

Publication (app → disk → Sidecar):

- `CodexTargetPath()` = `%LOCALAPPDATA%\OpenMPT\AI\codex-target.json`
  (`AIService.cpp:45-50`); Sidecar auto-target path
  (`openmpt_mcp.py:75-77`).
- `Panel::PublishActiveDocument` (`AIService.cpp:519-580`): refuses while the panel
  has retained work (`:526-530`), requires an active document (`:532-536`), writes
  `{version:1, pipe, instance, document, generation(UUID), title}` atomically via
  temp file + `MoveFileExW` (`:561-576`).
- Sidecar `refresh_target` (`openmpt_mcp.py:101-135`) validates version/size/identity,
  latches `notAttached`/`schemaFailure`, refuses target switch while
  `retained_session` unless the tool is an ending tool, and clears the latch when a
  new generation appears. `attachment_error` stays latched until re-publication
  (`:185-205`).
- No foreground-document fallback: explicit `--pipe/--instance/--document` or the
  published target file only (`openmpt_mcp.py:236-255`; `sidecar/README.md:90-140`).

Token capture:

- App: token generated in `Capture` (`AIPattern.cpp:95-100`); connection ownership
  captured as `capabilityConnection` when the result carries a session
  (`AIService.cpp:638-640`); other connections always receive `busy`
  (`:626-627`) even with the correct token.
- Sidecar: `update_session_state` sets `retained_session = true` on
  `get_pattern_context` with a `session`, clears it on ending tools or
  `occupancyLost`/`documentGone`/`instanceGone` (`openmpt_mcp.py:137-144`).
  It does not validate or store the token.
- Multi-instance sharing of the single target file is documented
  (`sidecar/README.md:135-137`).

---

## 9. Requirement-by-requirement mapping

| Requirement | Existing seam | Missing / gap |
| --- | --- | --- |
| `get_pattern_order` reads the current sequence, preserving duplicates, skips (`+++`), stops (`---`), invalid refs, plus unreferenced valid patterns | Data model and accessors: `Sndfile.h:520`, `ModSequence.h:26,156-163`, `ModSequence.cpp:254-259`, `PatternContainer.h:68,77`, `patternContainer.cpp:145-155`, `Snd_defs.h:30-31` | No tool exists. Needs a read-only execution path that never calls `SetAIOccupied` (`AIPattern.cpp:213,223`) and does not require `PatternView` (`AIService.cpp:630-632`). No existing enumeration of unreferenced valid patterns. Sidecar schema + allowlists + "five tools" tests must change. |
| `switch_pattern(pattern, optional session token)` validates ownership | Connection check `AIService.cpp:626-627`; token check `AIPattern.cpp:207`; `capabilityConnection` capture `:638-640` | A new tool must be allowed in `AIPattern.cpp:199-200` and the sidecar `TOOLS`; the existing `boundPatternViolation` (`:202-203`) means the tool cannot be implemented inside the current binding. |
| Rejects dirty/pending proposals | `approvalPending` (`AIPattern.cpp:204`), `busy` (`:212`), `candidateExists` (`:231`), `stale` (`:208`), `Review().status` (`:388`) | No `IsModified`/document-dirty gate exists in the AI facade. "Dirty" needs a concrete definition (see §15). |
| Requests approval; approval wait/cancel/timeout semantics | `m_pending` + `pending_approval` (`AIPattern.cpp:341-345`), `Panel::pending` (`AIService.cpp:779`), ReviewPanel buttons (`:958-961`), `Tick` pause (`AIPattern.cpp:80-83`), cancellation paths (`AIService.cpp:513-517,595-602,745,1190`) | Pending is replace-specific; `ResolveExpansion` and `ExpansionRange` cannot address a switch. UI Approve/Decline labels/state assume expansion (`AIService.cpp:799-800,903-909`). |
| Rebinds full-pattern baseline/dependencies after validation | `Capture` full baseline + `Signature` + whole-pattern envelope (`AIPattern.cpp:84-101`) | `m_pattern` is const (`AIPattern.h:42`); no rebind API; token regenerated by `Capture`, so preserving the token across a rebind needs new code. `Panel` could swap the `unique_ptr` (`AIService.cpp:379,633`) but must update `capabilityConnection`/`document`. |
| Syncs Patterns display and clears selection without changing sequence/order/play position | `SetCurrentPattern` (`View_pat.cpp:214-258`), cursor/selection (`View_pat.h:260-263`), review-evidence exemplar (`AIService.cpp:972-974`) | None if the Patterns view already exists; if it does not, `ActivateView(IDD_CONTROL_PATTERNS, pattern)` can reselect sequence/order (`Ctrl_pat.cpp:592-625`) — activation itself must be handled deliberately. |
| Same target no-op | Analogous early-return in `COrderList::SetCurSel` (`Ctrl_seq.cpp:352-353`); same-pattern reads pass the bound check (`AIPattern.cpp:202-203`) | No `switch_pattern` no-op exists; must return `ok` without a new `Capture`/token/display change. |
| Manual navigation does not rebind | `m_pattern` const (`AIPattern.h:42`); only session-less context reads sample the view (`AIService.cpp:628-633`); `boundPatternViolation` (`AIPattern.cpp:202-203`); ticket 35 AC1 | Nothing to build; must avoid introducing a view→capability sync that rebinds on `CTRLMSG_PATTERNCHANGED`. |
| Independent persistent default-off `always-allow-switch` and `always-accept` settings | Section/key pattern `AI/MCP` (`AIService.cpp:436-439,677-679`); `Configure` (`AIPattern.cpp:66-70`); checkbox creation/layout (`AIService.cpp:433-445,485-511`) | `Configure` has only `alwaysApprove`; new flags and keys needed. "Enabling handles pending requests" has no precedent (the existing toggle only affects later requests). |
| Auto apply uses the existing atomic path and retains pending proposals with explicit errors on failure | `Apply` (`AIPattern.cpp:399-425`), `simulateFailure` (`:411`), failure tests (`mpt_tests_ai_pattern.cpp:119-127`) | None structurally; a new always-accept path should call `Apply()` and surface its typed errors, leaving `m_proposal` intact. |
| Single-pattern proposal and Undo | `m_proposal` per capability; `PrepareUndo` whole pattern (`AIPattern.cpp:415`); one-Undo tests (`mpt_tests_ai_pattern.cpp:129-133`) | None. |

---

## 10. Test seams

In-process native facade tests (fastest seam for new `PatternCapability` logic):

- Entry: `OPENMPT_AI_TEST_FIXTURE` → `Test::PianoRollPatternTests` +
  `Test::AIPatternTests` (`Mptrack.cpp:1631-1643`), result written to
  `OPENMPT_AI_TEST_REPORT` as `PASS`/`FAIL: ...`; the demo JSON is written from
  `mpt_tests_ai_pattern.cpp:115-116` when `OPENMPT_AI_DEMO_REPORT` is set.
- Suite file: `openmpt-original_ref/test/mpt_tests_ai_pattern.cpp`
  (`void AIPatternTests` at `:30`); it can freely mutate the in-memory document
  (e.g. sequence/pattern edits for stale and order tests). `test.h:26` declares it;
  build inclusion at `build/vs2022win10/OpenMPT.vcxproj:2456`.

Real app + real pipe integration seam:

- Gate: `OPENMPT_AI_ENDPOINT_REPORT` env (`Mptrack.cpp:1614-1630`); fixture via
  `OPENMPT_AI_TEST_FIXTURE`; `AI::IntegrationHost` (`AIService.cpp:1116-1143`)
  starts the panel/broker, activates the Patterns tab, starts playback, then the
  timer harness binds `OPENMPT_AI_PATTERN` (`AIService.cpp:711-713`), disables
  follow-song (`:716-720`), and atomically writes the endpoint JSON
  `{pipe, instance, document}` after two stable ticks (`:731-738`).
- Shutdown: `OPENMPT_AI_STOP_FILE` or the 60 s test deadline (`:702-706`).
- Dispatch-time liveness hook: envelope field `test_close_before_dispatch`
  (`AIService.cpp:765-775`).
- Python driver: `sidecar/test_native_integration.py` (`OPENMPT_RUN_NATIVE_INTEGRATION=1`
  at `:21`, `OPENMPT_TEST_EXE` override `:16-20`); it starts the built
  `OpenMPT.exe`, talks through the real Sidecar, and uses `probe.PipeClient`
  (`sidecar/probe.py:27-83`) for raw envelope tests. The `tools/list == 5`
  assertion is at `test_native_integration.py:175`.
- Audio-callback IPC instrumentation: `AudioCallbackIpcCheck`
  (`AIService.cpp:1099-1108`), trace assertion helper in
  `test_native_integration.py:54-58`.

Sidecar process-boundary seam:

- `sidecar/test_sidecar.py` `EndpointDouble` (a real Windows named-pipe peer,
  `:28-105`); exactly-five-tools assertion `:110-128`; translation test
  `:256-269`; target pinning tests `:149-186`.
- `sidecar/test_probe.py` covers the probe CLI against the same double.

Tests that will fail when the tool count changes and must be updated:
`sidecar/test_sidecar.py:110-128`, `sidecar/test_sidecar.py:256-269`,
`sidecar/test_native_integration.py:175`; docs `sidecar/README.md:41,63,119,198`
and `USER_GUIDE.md:192`.

---

## 11. Build and test commands

| Command | Purpose | Evidence / output |
| --- | --- | --- |
| `.\build-local.ps1` | Build only: `vswhere` → MSBuild `openmpt-original_ref\build\vs2022win10\OpenMPT.vcxproj`, `Debug|x64`, `WindowsTargetPlatformVersion=10.0.26100.0` | `build-local.ps1:1-10` |
| `.\build-local.ps1 -Test` | Build + native `AIPatternTests`/`PianoRollPatternTests` on `test-fixtures\ai-collab-fixture.mptm` + optional real-project Piano Roll regression; expects `PASS` in `.scratch\ai-native-test.txt` and demo in `.scratch\ai-pattern-demo.json` | `build-local.ps1:11-45` |
| `python -m unittest discover -s sidecar -v` | Sidecar/probe process tests | ran here: **31 tests, OK, 4 skipped** (native opt-in) |
| `$env:OPENMPT_RUN_NATIVE_INTEGRATION='1'; python -m unittest sidecar.test_native_integration -v` | Real app + pipe + sidecar | documented `sidecar/README.md:38-45`, `test_native_integration.py:21` |
| `python test-fixtures/validate_fixture.py` | 131 structural/playback checks | ran here: **All 131 checks passed** |
| `python sidecar/probe.py demo --pipe ... --instance ... --document ... --drift-check` | Owner probe / guided failures | `sidecar/README.md:66-84` |

Built binary used by native tests:
`openmpt-original_ref\bin\debug\vs2022-win10-static\amd64\OpenMPT.exe`
(exists, timestamp 2026-09-13 16:27; `libopenmpt_test.exe` and `openmpt123.exe` also
present). Native suite/runners were **not** re-run during this exploration to keep
the report the only artifact; prior ticket logs (`issues/29`, `issues/32`) record
`PASS`.

---

## 12. Fixtures

- `test-fixtures/ai-collab-fixture.mptm` — purpose-built issue-28 fixture:
  2 patterns, 128 rows, 4 channels, order list `0, 1` (each target pattern exactly
  once), 3 instruments/samples, fixed 125 BPM / 4-4 timing. Details
  `test-fixtures/README.md`; validated by `python test-fixtures/validate_fixture.py`
  (131 checks, ran OK).
- `test-fixtures/th04_15_betafinalmix.mptm` — **untracked** real-project module used
  only by the Piano Roll regression (`build-local.ps1:29-40`; harness
  `Mptrack.cpp:1645-1657`; test `mpt_tests_pianoroll.cpp:215+`).
- `test-fixtures/generate_fixture.py` (deterministic generator) and
  `validate_fixture.py` (independent parser).
- **Fixture gap:** no fixture contains duplicate order items, `+++`, `---`, or
  invalid pattern references; the validator asserts the opposite ("each target
  Pattern exactly once", `test-fixtures/validate_fixture.py` module docstring,
  `test-fixtures/README.md`). `get_pattern_order` edge cases must be exercised by
  mutating `sf.Order()` in the in-process native test (which is already the style
  used for stale tests in `mpt_tests_ai_pattern.cpp:145-160`).

---

## 13. Relevant instructions / decision records

- `AGENTS.md` (workspace): documentation format = local Markdown; default label
  `triage`; keep guidance concise and repo-specific.
- `CONTEXT.md` domain vocabulary that constrains behavior: Document Revision
  (`:15-17`), Change Proposal (`:31-33`), Review Unit (`:35-37`),
  Application Capability (`:39-41`), Capability Mode (`:55-57`), Agent Edit Session
  (`:59-61`), MCP Sidecar (`:63-65`), Operation Receipt (`:67-69`).
- `.scratch/ai-collaborative-openmpt/map.md` "Decisions so far" (especially
  issue 20 "single-Pattern proposal loop", issue 23 "keep candidate private … reserve
  atomic Apply plus one ordinary Undo", issue 25, and "Not yet specified").
- Resolved tickets that fixed the current contract:
  `issues/29-pattern-capability-seam-five-tools.md` (five tools, occupancy, signature,
  envelope, stale, Apply), `issues/32-proposal-review-ui-atomic-apply.md` (review UI,
  Apply/Undo, settings row), `issues/30`, `issues/31` (broker, sidecar, pinning).
- `issues/35-fix-retained-occupancy-owner-ui-usability.md` AC1/AC2 are the recorded
  decisions closest to the new feature: manual Pattern navigation during occupancy is
  allowed and the agent session stays pinned; native Play/Pause/Stop must remain
  usable. Ticket status is still "open" in `.scratch`, though the code/git log show
  ticket 35-1/35-2 implemented; code comments reference a later "Issue 36" whose
  ticket document is not present in `.scratch/ai-collaborative-openmpt/issues/`.
- `.agents/skills/openmpt-pattern-mcp/SKILL.md` + `references/*` define the
  agent-facing contract (route, operating rules, session lifecycle); changing the
  tool set requires updating these files.
- `sidecar/README.md` documents envelope, pinning, `--auto-target` behavior, and the
  "five tools"; `USER_GUIDE.md:147-238` documents the owner-facing AI/MCP UI.

---

## 14. Preexisting dirty / untracked files (preserve; do not sweep)

`git status --porcelain` at HEAD `a83f7bc27`:

| Path | State | Nature |
| --- | --- | --- |
| `OpenMPT.exe.lnk` | deleted (tracked) | leftover shortcut removal |
| `build-local.ps1` | modified | adds `OPENMPT_PIANOROLL_REGRESSION_FIXTURE` run to `-Test` |
| `openmpt-original_ref/mptrack/Mptrack.cpp` | modified | adds the Piano Roll real-project regression harness branch |
| `openmpt-original_ref/mptrack/PianoRollPattern.cpp` | modified | Piano Roll channel/layer assignment rewrite (conflict-free layer placement) |
| `openmpt-original_ref/test/mpt_tests_pianoroll.cpp` | modified | adds `PianoRollRealProjectTests` |
| `openmpt-original_ref/test/test.h` | modified | declares `PianoRollRealProjectTests` |
| `.agents/` | untracked | skill package for the MCP tools |
| `JuceOPLVSTi-master/` | untracked | unrelated VST source |
| `OpenMPT.exe - 快捷方式.lnk` | untracked | shortcut |
| `USER_GUIDE.md` | untracked | owner guide (contains the AI/MCP UI tables) |
| `test-fixtures/th04_15_betafinalmix.mptm` | untracked | real-project Piano Roll regression fixture |

`git diff --stat`: 6 files changed, 164 insertions(+), 16 deletions(-). None of these
changes touch `AIPattern.*`, `AIService.*`, or `sidecar/*`; a new feature branch can
build on them as-is.

---

## 15. Conflicts and uncertain interpretations

1. **Ticket 35 status vs. implementation.** `issues/35-...md` still says
   "Status: open", but git history (`b0e1509a3`/`d3ef7c12c`/`e7547c61a` "ticket 35-1",
   "35-2") and `AIService.cpp` comments ("Issue 36") show the UI was reworked. There
   is no `36-*` ticket file under `.scratch/ai-collaborative-openmpt/issues/`.
2. **`USER_GUIDE.md` contradicts the new feature.** `:237` instructs the owner to end
   the Agent session before switching Pattern; the requirement is a session-preserving
   switch.
3. **`sidecar/README.md:198`** states "Pattern indices are app-owned; there is no
   Pattern selector on this surface" — the new `switch_pattern` is exactly a Pattern
   selector, and the five-tool contract is asserted by tests/docs.
4. **What "dirty" means.** Candidates: `CModDoc::IsModified()` (unsaved edits),
   `Signature() != m_signature` (dependency drift → `stale`), candidate ≠ baseline
   (`candidateExists`), or an existing handoff proposal. The requirement text says
   "rejects dirty/pending proposals", which most likely bundles
   `m_pending` + `m_proposal` + un-handed candidate edits; `IsModified()` is a
   separate unsaved-document concept with no current AI gate.
5. **Where `switch_pattern` lives.** Because of `const m_pattern` and the
   `boundPatternViolation` guard, it is a facade/Panel-level operation rather than a
   `PatternCapability::Dispatch` case inside the old binding. The C++ "public
   capability" for the new tool is therefore a design choice: a new Panel-mediated
   method (`Panel::SwitchPattern`) that replaces the capability, or a new
   `PatternCapability::Rebind`/`Switch` method. No such seam exists today.

---

## 16. Remaining unknowns / evidence gaps

- **No native or real-pipe run was performed in this exploration.** The built
  `OpenMPT.exe` exists, and the commands are documented/reproducible, but current
  tree native test status was not re-verified to avoid writing non-declared artifacts.
  Prior ticket logs report `PASS`.
- **No design decision is recorded** for the two new settings' exact key names,
  checkbox texts, whether switch/accept approvals share the existing
  Approve/Decline buttons, or whether `switch_pattern` is a sixth/seventh tool on the
  same "Pattern mode" surface. No ticket exists for this feature in
  `.scratch/ai-collaborative-openmpt/issues/` (the highest-numbered ticket is 35).
- **Token semantics on rebind are unspecified.** Existing `Capture()` mints a new GUID
  token; the requirement's "optional session token" implies continuation. There is no
  existing API to preserve `m_token`/`m_signature` across a pattern change.
- **"Unreferenced valid patterns" result shape is unspecified** (field names, whether
  the order list is returned with semantic tags for skip/stop/invalid, whether
  duplicates are flagged or left raw). The existing Score Context JSON shape
  (`AIPattern.cpp:138-174`) and sidecar schema conventions are the nearest guides.
- **`get_pattern_order` availability without the Patterns tab** is not decided by any
  existing code: the current session-less read requires `PatternView`
  (`AIService.cpp:630-632`), while an order read does not inherently need the view.
