# AI-MPT User Guide

English · [简体中文](USER_GUIDE_CHN.md)

This guide is for music creators using the custom OpenMPT build in this project. It covers two workflows: viewing or editing Tracker Patterns in the Piano Roll, and having an MCP-capable Agent generate candidate changes for the user to review. Editing in the Piano Roll is off by default; the editing features are currently experimental, must be enabled manually, and require confirmation.

The current version still uses **Tracker Pattern** as the only source of song data. The Piano Roll is a graphical editing view of it; the Agent only generates Pattern modification proposals inside a private candidate, and by default the user must apply them in the review area. Only after the user explicitly enables the persistence option **Always accept submissions** are frozen proposals applied automatically once they pass the revision check, cell validation, and Undo preparation; if automatic application fails, the document is left unchanged and the proposal is retained for manual handling.

## I. The Piano Roll window

### 1. Opening the window and getting to know the interface

Open a module file, then select the **Piano Roll** tab in the document control area.

The upper control area contains:

| Control | Purpose |
| --- | --- |
| `Pattern` | Selects the Pattern currently displayed and edited. It uses the same Pattern number as the traditional Patterns tab. |
| `Instrument` | Selects the instrument used for newly created notes; in modules without instruments, it selects the Sample. |
| `Snap` | Turns row snapping on or off. |
| `1 / 2 / 4 / 8` | Sets the number of rows to snap to, which also determines the default length of notes created by double-clicking. |
| `Allow editing` | Off by default. Only after you check it and confirm the experimental editing prompt are note writes allowed in the Piano Roll; unchecking it returns to read-only browsing. |
| `Split by instrument` | Available only after editing is enabled; reorganizes channels by instrument and note overlap. This operation can affect multiple Patterns across the entire document. |
| `Undo / Redo` | Undoes or redoes Pattern modifications made in the Piano Roll. |
| `Play / Stop` | Plays the current Pattern or stops playback. |
| `Follow Song` | During playback, keeps the horizontal view following the current playback row. |

Horizontally, the canvas represents Pattern rows and time; vertically, it represents pitch. The top ruler shows row numbers and beats, with bars and beat lines drawn in different strengths; the left piano keyboard and the top ruler remain visible while scrolling. Note colors distinguish instruments, and wider notes display the instrument name along with existing volume and effect information.

By default you can view, select, copy, and preview notes, but you cannot insert, drag, resize, delete, cut, paste, or run `Split by instrument`. `Undo / Redo` are not governed by this switch; if there are undoable records, they may still modify the document.

### 2. Enabling experimental editing

Before editing, save a working copy and check every channel: **each channel should use only one instrument or Sample**. This is a prerequisite for using the current editing features; if a channel mixes multiple instruments or Samples, inspect and tidy it on the traditional **Patterns** tab first. `Split by instrument` can only be clicked after editing is enabled, and since it may change the entire document, it is not a suitable preparation step before enabling editing.

After checking `Allow editing`, the program shows an **Experimental Piano Roll Editing** confirmation dialog warning that the feature is still buggy and that each channel must use only one instrument or Sample. The checkbox stays checked and editing opens only if you allow it; cancelling the confirmation keeps the view read-only. When you are done, you can uncheck it to return to read-only browsing. This switch controls only Piano Roll editing; it does not determine whether the Agent can edit.

### 3. Creating and previewing notes

1. Enable `Allow editing` as described in the previous section.
2. Select an existing instrument or Sample in `Instrument`.
3. Turn on `Snap` if needed and choose 1, 2, 4, or 8 rows.
4. Double-click on the empty grid to create a note at the corresponding row and pitch.

A new note starts at the snapped row by default; with snapping on, its initial length equals the snap row count, otherwise its length is 1 row.

Preview options:

- Click a piano key on the left to preview the pitch on the current instrument.
- With the canvas focused, the key row `Z S X D C V G B H N J M` previews the twelve semitones starting from C-5.
- Releasing the key or mouse button stops the current preview. Keyboard preview is currently for auditioning only and does not record into the Pattern.

### 4. Selecting notes

- Click a note: selects only that note.
- `Ctrl` + click: adds a note to the selection, or removes it from it.
- Hold `Shift` and drag a selection box on an empty area: box-selects intersecting notes.
- `Ctrl` + `Shift` drag-box: appends the notes inside the box to the existing selection.
- `Ctrl+A`: selects all notes in the current Piano Roll projection.

Selected notes are drawn with a high-contrast double border. Switching Pattern clears the current selection.

### 5. Moving, transposing, and resizing notes

- Drag the body of selected notes: changes both time position and pitch; with multiple notes selected, they move as a group.
- Drag a note's left edge: changes the starting row while keeping the end position fixed.
- Drag a note's right edge: changes the ending row.
- `←` / `→`: move the selected notes by 1 row, unaffected by the Snap setting.
- `↑` / `↓`: raise or lower the selected notes by 1 semitone.

Mouse drag positions apply the current Snap setting. If an operation would go beyond the Pattern's row range or the pitch range supported by the module format, the modification is rejected and the reason is shown in the status bar.

Note lengths can be written reliably only in module formats that support explicit Note Off events. If the current format does not, the reason is shown at the top of the canvas and resizing via the left and right edges is unavailable.

### 6. Deleting, copying, and pasting

- Right-click a note: deletes that note; if it belongs to a multi-selection, the whole selection is deleted.
- `Delete` or `Backspace`: deletes all selected notes.
- `Ctrl+C`: copies the selection.
- `Ctrl+X`: cuts the selection.
- `Ctrl+V`: pastes anchored to the row and pitch of the current canvas cursor.

The Piano Roll uses its own internal clipboard and does not share formats with the traditional Pattern editor or the Windows system clipboard. Copied content stores relative time, pitch, channel, instrument, volume, and length; after pasting, notes are still placed according to the current channel routing rules.

### 7. Scrolling and zooming

- Mouse wheel and scroll bars: browse the canvas.
- `Ctrl` + wheel: changes the horizontal width per row, i.e. time-axis zoom.
- `Shift` + wheel: changes the vertical height of keys and pitch rows.
- Click the top ruler: moves the paste and operation cursor to the corresponding row.

With `Follow Song` enabled, when playback reaches the current Pattern, the view automatically keeps the playback row on screen. You can temporarily disable it to browse other positions freely.

### 8. Channel routing and Tracker data

The Piano Roll displays the pitched notes in a Pattern. Note lengths are derived from subsequent notes in the same channel, Note Off, Note Cut, Note Fade, or related terminating effects.

To allow overlapping notes of the same instrument to sound simultaneously, the Piano Roll writes back to the Tracker using the following rules:

1. Build channel groups per instrument.
2. Non-overlapping notes of the same instrument preferentially reuse the lowest available channel.
3. Overlapping notes fall to additional layers; more channels are added when necessary.
4. Write a Note Off at the note's end position when the format supports it.

This reassignment rule stays consistent while editing in the Piano Roll and may reorganize the note channels of other Patterns in the document. `Split by instrument` explicitly performs the same reorganization across the whole document and is available only when `Allow editing` is on. The operation enters OpenMPT's native Undo as a single unit; if Undo cannot be fully prepared, the format's maximum channel count would be exceeded, or Tracker-only data that cannot be safely represented would be overwritten, the entire operation fails without writing partial results. Because the editing features are still experimental, after performing this operation you should return to the Patterns tab to inspect the channel contents and preview the result.

Effect commands, special notes, and non-standard volume commands remain Tracker-only data. The Piano Roll tries to preserve them; when they conflict with the target note positions, the modification is rejected and the reason is shown. If this happens, go back to the **Patterns** tab and inspect the corresponding cells.

### 9. Using the Piano Roll alongside an Agent session

While the Agent holds an editing session, the Piano Roll shows a read-only notice. You can still scroll, view, preview, and play, but write operations are blocked, even if `Allow editing` is on. After the Agent session is completed, cancelled, or force-released, manual editing is still subject to the switch and channel prerequisites described above.

## II. Agent Collaboration

### 1. Configuring the Agent Client for the First Time

The current MCP Sidecar requires Windows and Python 3.10 or higher. First find the absolute path to this project's `sidecar/openmpt_mcp.py`; the path `C:/absolute/path/to/OpenMPT-for-AI/sidecar/openmpt_mcp.py` used in the examples below is a placeholder that must be replaced with the actual path on your own computer. Configuring the `--auto-target` mode once is recommended.

**Codex:** In the user-level `~/.codex/config.toml`, or in the `.codex/config.toml` of a trusted project, add:

```toml
[mcp_servers.openmpt]
command = "python"
args = ["C:/absolute/path/to/OpenMPT-for-AI/sidecar/openmpt_mcp.py", "--auto-target"]
required = false
startup_timeout_sec = 10
tool_timeout_sec = 300
```

You can also run this once:

```powershell
codex mcp add openmpt -- python "C:/absolute/path/to/OpenMPT-for-AI/sidecar/openmpt_mcp.py" --auto-target
```

**Other Agent software that supports local MCP stdio servers:** In the software's MCP server settings, add a local server named `openmpt`, choose the **stdio / command** type, and fill in the following fields. The configuration entry point and field names may differ between programs; what is shown here is the meaning of the fields, not a universal configuration file syntax.

| Field | What to enter |
| --- | --- |
| Launch command (command) | `python`, or the path to a Python 3.10+ executable the software can find. |
| Arguments (args, in order) | ① The absolute path to this project's `sidecar/openmpt_mcp.py`; ② `--auto-target`. The path is **one complete argument**; do not manually split it when it contains spaces. |
| Transport | `stdio`. The Agent software starts this Sidecar as a local subprocess; do not enter an HTTP URL. |
| Tool call timeout (if available) | At least 300 seconds is recommended, to allow for manual range expansion or Pattern switching approvals in OpenMPT. |

If the software only accepts JSON configuration, the example below merely shows the common field correspondence; adjust the outer key names and placement according to that software's own documentation:

```json
{
  "mcpServers": {
    "openmpt": {
      "command": "python",
      "args": ["C:/absolute/path/to/OpenMPT-for-AI/sidecar/openmpt_mcp.py", "--auto-target"]
    }
  }
}
```

After configuring, reload the MCP server or restart the client as required by the client. Then click `Connect active doc to Agent` on OpenMPT's **AI / MCP** page; it publishes the current document as the target, which local Agent clients using `--auto-target` can read. When switching documents, simply click this button again; there is no need to change the server launch arguments. The client should run locally; if it can only connect to remote HTTP MCP servers, it cannot use this Sidecar's stdio configuration directly.

### 2. Preparing the Agent's Target and Range

Before starting each collaboration session:

1. Open the module file to be worked on.
2. To specify a target Pattern or limit the range, go to the traditional **Patterns** page, select the Pattern, and drag-select the corresponding rows and channels.
3. If `Always allow Pattern switching` is enabled and you want to use the first Pattern in the Sequence, you can skip selecting a range and do not need to open the Patterns page beforehand.
4. Open the **AI / MCP** page.
5. Confirm the status at the top reads `Status: MCP ready`.
6. Click `Connect active doc to Agent`.

On its first read, the Agent usually binds the **current Pattern and selection on the Patterns page**; if `Always allow Pattern switching` is enabled and no range was selected, it instead binds the first valid Order Pattern of the current Sequence and is authorized for the entire Pattern. Note selections in the Piano Roll never become the Agent's authorized range. Merely switching window focus also does not automatically change the Agent target; the document published by the most recent click of the connect button is the current target.

Afterwards, to have the Agent edit another Pattern in the same document, no reconnection is needed: let the Agent request a switch and approve it on the AI / MCP page. Manual Pattern navigation on the Patterns page or in the Piano Roll does not change the Agent's binding target.

If the connect button is rejected, first check:

- Whether `Enable MCP` is on and the status has changed to ready.
- Whether there is an active document.
- Whether the document has had its Patterns page opened at least once; to skip this step, `Always allow Pattern switching` must be enabled and the current Sequence must contain at least one valid Pattern.
- Whether there is still an unfinished Agent session or a proposal awaiting review.

### 3. The AI / MCP Page

This page is divided into an upper and a lower section.

The upper section is for connection and settings:

| Control | Purpose |
| --- | --- |
| `Enable MCP` | Starts or stops the local MCP service. Enabled by default. |
| `Always approve range expansion` | Automatically approves Agent edits beyond the initial selection range. Disabled by default. |
| `Always allow Pattern switching` | Automatically approves subsequent legitimate Pattern switching requests. Disabled by default and independent of the range expansion setting; when enabled while a switch request is already waiting, it immediately attempts to approve it. With no selected range, the Agent binds the first valid Order Pattern of the current Sequence by default and takes the range of the entire Pattern. |
| `Always accept submissions` | Attempts to apply immediately after a submission is frozen. Disabled by default and independent of the two options above; when enabled while a proposal is awaiting review, it immediately attempts to apply it once. |
| Seconds input box | Sets the idle session timeout, from 1–3600 seconds, default 300 seconds. |
| `Save settings (seconds)` | Saves the switches and timeout settings above. |
| `Connect active doc to Agent` | Explicitly publishes the current active document as the target for Agent clients using `--auto-target`. |
| Status text area | Shows the service status, Pipe, instance ID, Agent target directory, and all open documents. |

The lower section is for session and proposal review:

| Control | Purpose |
| --- | --- |
| `RELEASE AI NOW` | Emergency release of the current Agent occupancy; in-progress calls receive a session-lost result. |
| `Approve expansion` | Approves this range request for the Agent to edit beyond the initial selection. |
| `Decline expansion` | Declines this range expansion. |
| `Approve Pattern switch` | Approves the pending Pattern switch request. |
| `Reject Pattern switch` | Rejects the pending Pattern switch request, keeping the original binding. |
| `Apply whole proposal` | Applies the complete proposal to the document as a single atomic change. |
| `Reject whole proposal` | Discards the complete proposal. |
| Change list | Shows original values, proposed values, and current document values by row and channel. |
| Graphical evidence area | Displays the pitch distribution of Baseline, Proposal, and Current document side by side. |

Partial acceptance is currently unavailable: only the whole proposal can be applied or rejected at a time. Each proposal and each Pattern Undo involves only one Pattern; switching Sequence or multi-Pattern drafts is not currently supported. The Agent can create a missing Pattern when a Pattern switch is approved, and can reorder the current Sequence's Order while holding the session.

### 4. Giving Tasks to the Agent

After connecting, you can describe the musical goal directly in an Agent task. State the target voices, row range, instruments, and what should be preserved as clearly as possible, for example:

```text
Read the current Pattern and add a two-voice harmony on channels 2 and 3. Keep the melody on channel 1 and the bass on channel 4 unchanged, and hand the result to me for review when done.
```

```text
Analyze the first 32 rows of the current Pattern and transpose the melody in the selected channels up one octave as a whole; preserve all effect and volume information, then submit a proposal.
```

```text
Only analyze the current selection and describe the rhythm, tonality, and voice relationships; do not make any changes.
```

The current Pattern MCP provides eight capabilities to the Agent. Order entry indices and Pattern numbers both start at 0:

| Capability | User-visible meaning |
| --- | --- |
| Read Pattern order | Lists every Order entry of the current Sequence, preserving duplicate references, skips, stops, and invalid references, and lists valid Patterns that are not referenced. Read-only: it does not request occupancy and does not require the Patterns page to be open. |
| Reorder / insert into Pattern order | While holding the session, submits a complete permutation of the current Order indices and may add `{pattern: N}` at a target position to insert valid Pattern N, not yet referenced by the current Sequence, into the Order. Every original index must still appear exactly once, so existing duplicate Patterns, `+++`, `---`, and invalid entries are moved as-is, never deleted or rewritten; the operation is applied immediately and keeps the current Pattern binding and token. |
| Switch bound Pattern | The Agent requests switching the edit target to the Pattern with the given number. While holding a session it must carry the current token; without a session it may request directly, and approval establishes a new Agent session. After approval, the target Pattern is fully recaptured as the baseline and a new token is returned, with all rows and channels authorized. If a target within the format's range does not exist and the Order still has capacity, approval first creates the target using the source Pattern's row count and appends it to the valid end of the current Order. |
| Read Pattern context | Retrieves the current candidate or original baseline, tempo, format, instruments, sparse cells, and, for the current format, the writable special notes, volume/effect command numbers and parameter ranges. |
| Replace a contiguous single-channel segment | Accumulates changes in the private candidate without writing directly to the document; can create, change, or clear ordinary notes supported by the current module format, Note Off (`===`), Note Cut (`^^^`), Note Fade (`~~~`), and all volume-column and effect-column commands. Special notes follow the catalog published in the read context. |
| Submit for review | Freezes the complete candidate and releases Agent occupancy. By default it goes to manual review; with "Always accept submissions" enabled, it immediately attempts an atomic apply. |
| Cancel session | Discards the entire candidate. |
| Release read-only occupancy | Ends an analysis session that made no changes. |

### 5. Session Occupancy and Range Approval

Once the Agent starts editing, the status in the lower section shows `AI OCCUPIED`. At this point:

- You can navigate Patterns, scroll the view, play, and preview notes.
- Write operations on the Patterns page and in the Piano Roll are blocked.
- Every successful read or write refreshes the idle timeout.
- If any of the document's relevant Pattern, instrument, Sample, or tempo dependencies change outside the session, the old candidate is judged stale and the Agent must read again.

If the Agent requests to modify rows or channels outside the initial selection, the lower section shows the requested range and the currently authorized range. While an approval is pending, the timeout counting pauses; a waiting call returns its final result after approval or rejection (a successful replacement result or a typed failure such as `rangeRejected`) and never returns a pending status on its own:

- Click `Approve expansion` when the range is reasonable; the original call continues.
- Click `Decline expansion` to keep edits strictly limited to the original selection.
- If you often work on whole Patterns and do not want to confirm every time, enable `Always approve range expansion`; this setting is saved for later sessions.

If the Agent requests a switch to another Pattern, the lower section shows the source Pattern, the target number and name, and the full-Pattern (all rows, all channels) authorization range that approval would grant:

- After you click `Approve Pattern switch`, the target is recaptured as a brand-new baseline and a new token is returned; the original binding and the old token become invalid.
- Clicking `Reject Pattern switch` keeps the original binding; when the request came from an Agent already holding a session, the token and candidate stay unchanged and the idle timeout is refreshed; otherwise the waiting reservation is released.
- With `Always allow Pattern switching` enabled, legitimate requests are approved automatically; if a request is already waiting when the setting is enabled, it immediately attempts to approve it.
- With this setting enabled and no range selected, the first read does not require opening the Patterns page beforehand: the Agent binds the first valid Pattern in the current Sequence's Order, with all of its rows and channels as the initial range; if a genuine drag-selection exists, the Pattern and range of that selection still take priority.
- The idle timeout also pauses for a pending switch request; force release, disconnection, or closing the document terminates the wait, and approval re-validates the target and the session.

While there are uncommitted candidate changes or a proposal awaiting review, switching is rejected; you must first apply, reject, or cancel the current work. Switching to an existing Pattern does not modify the Sequence, Order contents, or playback position; creating a missing target appends one Order reference. Duplicate Order references still edit the same Pattern. Manual navigation on the Patterns page or in the Piano Roll does not change the Agent's bound target.

If the Agent switched to a Pattern that already exists but is not referenced by the current Sequence, the reorder tool can be called within the same retained session: the target array still contains all original Order indices, with `{pattern: N}` inserted where needed. This pins down the exact Pattern number and playback position without losing any existing Order entries. Only valid Patterns that the current Sequence genuinely does not reference may be inserted; if a Pattern is already in the Order, move its existing Order index instead of inserting it again by Pattern number.

If the Agent becomes unresponsive for a long time or you need to resume manual editing immediately, use `RELEASE AI NOW`. This operation clears the active session; if a proposal awaiting review has already been produced, handle it explicitly with `Apply whole proposal` or `Reject whole proposal`.

### 6. Reviewing and Applying Proposals

When the Agent finishes a candidate, it freezes the entire modification into a proposal for a single Pattern and releases write occupancy. Then check the returned status:

- `status: pending_review` (default): the proposal awaits manual review; the document content has not changed yet.
- `status: applied`: returned only when "Always accept submissions" is enabled and the proposal passes the revision check, cell validation, and Undo preparation; in this case the changes have already been applied to the document atomically.

A failed automatic application returns `ok: false` while the status remains `pending_review`: the document is unchanged, the proposal is retained for manual handling, and Agent occupancy has already ended. With "Always accept submissions" enabled, if a proposal awaiting review already exists, it immediately attempts to apply it once; a failure is not silently retried. Whether automatic or manual, each apply or rejection involves only the one Pattern of the current proposal, and a single Undo reverts only that Pattern.

Review steps:

1. Check `Proposal current` or `Proposal stale` in the status.
2. In the change list, examine each entry's row, channel, the notes before and after the change, and the current document values.
3. Clicking a list item moves the traditional Patterns page's cursor to the corresponding Pattern cell; switch back to the Patterns page to verify against the raw Tracker data.
4. Review the three-column graphical evidence below: Baseline, Proposal, and Current document.
5. Click `Apply whole proposal` when the whole proposal is correct; otherwise click `Reject whole proposal`.

The apply operation checks the document revision and all candidate cells again, prepares OpenMPT native Undo, and only then writes everything as a whole. Any check or Undo-preparation failure leaves the document unchanged; after an automatic application fails, the user can still handle the retained proposal with `Apply whole proposal` or `Reject whole proposal`. Even after a successful apply, playback verification is recommended; if the result does not sound as expected, use OpenMPT's Undo to revert.

If a proposal's status is stale, the document has changed in relevant ways. Do not apply the old plan directly; reject the proposal, reselect the range, and have the Agent generate a new proposal based on the latest content.

### 7. Switching Documents or Patterns

- Switching Pattern: you do not need to end the session first. Have the Agent request `switch_pattern`, then approve it on the AI / MCP page. While holding a session, the Agent must carry the current token; without a session, approval establishes a new Agent session. After approval, the target Pattern is fully recaptured as the baseline and a new token is returned; the old token becomes invalid. Switching to the current Pattern only keeps the original binding and refreshes the idle timeout; a missing Pattern within the format's range is created upon approval and appended to the Order.
- Manual navigation: switching Patterns on the Patterns page or in the Piano Roll changes only your view, not the Agent's bound target; only an approved switch changes it.
- Switching to an existing Pattern does not modify the Sequence, Order contents, or playback position; creating a missing target appends one Order reference. Duplicate Order references still point to the same Pattern.
- Switching documents: after completing or canceling the current work, activate the new document and click `Connect active doc to Agent` again.
- Multiple OpenMPT instances share the same current-user target file; the most recent connect-button click determines the Agent client's target.

While the Agent already holds a session, a newly published document does not take it over. The Agent should first submit, cancel, or release the old session, and only then start calls against the new target.

Pattern switching involves two separate things: "manual view navigation" and "Agent binding switch". Selecting a different Pattern in the Patterns tab or the Piano Roll only changes your own viewing position; the Agent remains bound to the original Pattern. The editing target changes only after the Agent calls `switch_pattern` and the request is approved.

One Agent session binds to exactly one Pattern. When switching, use the zero-based Pattern number from the Sequence; after approval, the target Pattern re-captures its full baseline and a new session token is returned, invalidating the old one. When the current Pattern appears at multiple Order positions, they still refer to the same Pattern and do not produce separate copies.

To edit several Patterns in a row, proceed in this order: first finish and review or cancel the candidate changes for the current Pattern, then request a switch; after confirming that `switched` is returned, re-read the target Pattern, complete verification, and then submit. Each proposal and each Undo act on only one Pattern; multiple Patterns cannot be merged into a single draft.

## III. Recommended Collaboration Workflow

1. Save a working copy; to write with the Piano Roll, first check each channel's instrument usage, then enable `Allow editing` and confirm the experimental prompt.
2. On the Patterns page, select the Agent's Pattern and initial range.
3. Connect the current document on the AI / MCP page.
4. Describe the musical goal and the parts that must be preserved to the Agent.
5. While the Agent works, play back and view, but do not attempt manual writes.
6. Approve or reject range expansions and Pattern switches one by one; when you trust the Agent, you can enable the respective "always allow" option for each.
7. In the review area, compare Baseline, Proposal, and Current document.
8. Apply or reject the whole proposal; with "Always accept submissions" enabled, check the returned `applied` or `pending_review` status instead.
9. Play back to verify the result, and use Undo or start the next round of collaboration as needed.

This workflow keeps manual Piano Roll editing, the Agent's private candidate, and the final document modification separate: the user always retains the target selection and the authorization decisions; unless the user explicitly enables "Always accept submissions", the final apply remains a manual action.
