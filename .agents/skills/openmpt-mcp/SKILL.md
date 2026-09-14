---
name: openmpt-mcp
description: Operate the OpenMPT Pattern MCP tools to read or reorder the current Sequence order, switch to or create the bound Pattern with approval, inspect Score Context, compose or revise tracker cells in a retained Agent Edit Session, and finish with handoff. Use when an OpenMPT document is connected and the task concerns the Sequence order or reading or editing its bound Tracker Pattern; do not use for UI automation, audio review, project-wide settings, or unsupported non-Pattern operations.
---

# OpenMPT MCP

Use the connected Pattern capability as a one-Pattern-at-a-time proposal workflow. Tracker-cell tools edit a private candidate; a frozen proposal is applied only after human approval or a saved automatic-accept preference. `reorder_pattern_order` is the explicit structural exception: it applies a validated permutation immediately inside a retained session. Read every result for its actual `status`.

## Tracker Pattern basics

A Pattern is a fixed-size grid of rows and channels. Each row is one step of musical time, and playback walks the rows in order; the ticks inside a row are where sliding and oscillating effects are applied (row timing is reported in `context.timing`). Every channel of a row is one cell, and each cell carries up to four logical columns:

- **Note** — The Note Column is used to trigger notes and to stop them in one way or another. Note events consist of a key and an octave, for example "C-5" (which is the Middle-C).
- **Instrument** — A note alone will of course not produce any sound, so in order to tell the program which instrument or sample should be played with the note, the instrument’s (if there are instruments) or sample’s (if there are only samples) index must be entered next to a note. For example, C-5 01 will play the Middle-C of the first sample or instrument. If there is no instrument number next to a note, the previously used sample or instrument is recalled using the previous volume and panning settings. Lone instrument numbers (without a note) will reset the instrument’s or sample’s properties like volume and default panning.
- **Volume** — This column is generally used to enter volume changes, but it can also be used for a couple of other effects in some formats. Not all formats support this column. Most notably, you cannot enter anything here in the MOD format.
- **Effect** — This is the most versatile column and can be used to enter a variety of commands that either affect the current note (such as pitch or volume changes), all notes on the channel (e.g. channel volume) or all channels (like tempo or global volume changes). This column is supported by all formats, but the number of supported commands varies between module formats.

In the Score Context a cell is not four text columns but six byte-valued fields: `note`, `instrument`, `volume_command`, `volume`, `effect_command`, `effect_parameter`. Tracker displays use letter notation (uppercase for the effect column, lowercase for the volume column, e.g. `G05` versus `g05`), but the tools use numeric command IDs with semantic names. Before authoring either command column, read `context.format.volume_commands` and `context.format.effect_commands` and use only IDs those catalogs publish; see [raw-cell-model.md](references/raw-cell-model.md) for the exact field rules.

The capability binds exactly one Pattern at a time, so a Pattern's rows and channels are the whole working surface of one session. Anything that spans Patterns is coordinated through the Sequence order instead (see [get-pattern-order.md](references/get-pattern-order.md)).

## Route

Load only the files needed for the current branch:

- If no document is attached or the target may be wrong, read [connect-target.md](references/connect-target.md).
- Before any retained read or edit, read [session-lifecycle.md](references/session-lifecycle.md).
- Before interpreting or constructing cells, read [raw-cell-model.md](references/raw-cell-model.md).
- Before authoring volume-command or effect-command cells, read the file for the bound format, identified by `context.format.name`:
  - `mod` → [effects-mod.md](references/effects-mod.md)
  - `xm` → [effects-xm.md](references/effects-xm.md)
  - `s3m` → [effects-s3m.md](references/effects-s3m.md)
  - `it` → [effects-it.md](references/effects-it.md)
  - `mptm` → [effects-mptm.md](references/effects-mptm.md)
  - `context.format.name` is always one of those five: a source format OpenMPT does not model natively (MED, DBM, OKT, …) is reported under the closest family, so there is no separate reference file for it.
- For a tool call, read exactly that tool's file:
  - [`get_pattern_order`](references/get-pattern-order.md)
  - [`reorder_pattern_order`](references/reorder-pattern-order.md)
  - [`switch_pattern`](references/switch-pattern.md)
  - [`get_pattern_context`](references/get-pattern-context.md)
  - [`replace_pattern_segment`](references/replace-pattern-segment.md)
  - [`handoff_for_review`](references/handoff-for-review.md)
  - [`abort_session`](references/abort-session.md)
  - [`release_occupancy`](references/release-occupancy.md)
- On any `ok: false` result, read [recover-errors.md](references/recover-errors.md) before the next mutation.

## Operating contract

1. Read context before reasoning about the music. The bound Pattern and zero-based coordinates come from OpenMPT, not from UI guesses.
2. The binding holds one Pattern. Manual navigation in OpenMPT never rebinds it; only an approved `switch_pattern` does.
3. For edits, acquire one opaque `session` with `get_pattern_context(occupy=true)`; an approved session-less `switch_pattern` also establishes a retained session on the target. Pass that exact token to every later call. While a session is retained, `switch_pattern` requires that token; approval returns a fresh token for the target.
4. Preserve unsupported raw fields and reconstruct every affected segment deliberately. A sparse omission inside a replacement range means an empty cell, not "leave unchanged."
5. Verify the candidate against the original baseline before finishing.
6. End every retained session exactly once with `handoff_for_review`, `abort_session`, or—only when no edits exist—`release_occupancy`.

Completion means the requested context was returned and read-only occupancy released, or an edited candidate was verified and an ending result proves the session ended: `ok: true` from any ending tool, or `status: pending_review` from `handoff_for_review`—even with `ok: false`, because that status proves the candidate was frozen and occupancy released. An ending call rejected before the freeze (for example `occupancyLost` or `stale`) proves nothing; re-read session state before claiming completion. Read the returned `status`: `applied` means the live document changed, `pending_review` means the proposal still waits for the human. Never promise that an automatic application will succeed.
