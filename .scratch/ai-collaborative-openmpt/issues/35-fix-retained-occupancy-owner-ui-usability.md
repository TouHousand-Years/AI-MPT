# 35: Fix retained-occupancy navigation, playback, proposal colours, and panel obstruction

Parent: 33-end-to-end-agent-loop-both-directions.md
Label: triage

**What to build:** Correct the owner-facing regressions found during the ticket-33 live session. Retained occupancy must block project mutation, Undo, and Redo without blocking Pattern navigation or the application's ordinary playback controls. The whole-proposal Piano Roll evidence must distinguish affected channels visually. The AI / MCP surface must remain discoverable and keep occupancy, release, approval, and proposal actions available without persistently covering a large part of the editing workspace.

**Blocked by:** None (defects reproduced during ticket-33 owner verification)

**Blocks:** 33 (End-to-end Agent loop: melody↔harmony both directions)

**Status:** open

## Observed during owner verification

The owner completed steps 1–5 of the ticket-33 verification instructions and reported:

1. After retained occupancy is acquired, the owner cannot switch Pattern.
2. A proposal containing changes in two channels is listed as two-channel work, but both channels use the same colour in the Piano Roll proposal preview.
3. OpenMPT's native Play and Pause buttons stop working during retained occupancy.
4. The AI / MCP window occupies too much of the editing workspace and is persistently obstructive.

These observations contradict ticket 33's requirement that navigation, inspection, and normal playback remain available during retained occupancy, and weaken the synchronized multi-voice review evidence required by ticket 32.

## Acceptance criteria

- [ ] During retained occupancy, the owner can switch between Patterns through the normal Pattern/Order UI; the Agent session remains pinned to its original Pattern, base revision, and initial write envelope.
- [ ] Native Play, Pause, and Stop controls remain usable during retained occupancy and play only committed document state, never the private candidate.
- [ ] Human project writes, Undo, and Redo remain blocked while the fixes above permit navigation and playback.
- [ ] In a proposal that changes two or more channels, the Baseline, Proposal, and Current-document Piano Roll evidence uses stable, visibly distinct per-channel colours, with enough labeling or legend evidence to identify the channel mapping.
- [ ] The AI / MCP surface no longer persistently obstructs the main Tracker/Piano Roll workspace. A compact, collapsible, docked, or equivalently non-obstructive presentation is acceptable, provided retained-occupancy state and the high-priority human release remain continuously visible and pending approval / Apply / Reject actions remain readily reachable.
- [ ] Automated regression coverage exercises the navigation/playback command gate and multi-channel preview colour assignment where practical; the owner repeats the relevant ticket-33 live checks for final interaction and visual acceptance.

## Non-goals

- No change to the five-tool MCP contract, proposal atomicity, session pinning, or candidate privacy.
- No audition of private candidates and no weakening of the retained-occupancy write/Undo/Redo gate.
- No general redesign of OpenMPT's window or docking framework beyond what is needed to make this focused surface non-obstructive.
