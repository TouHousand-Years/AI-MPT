# Documentation slice result: cross-Pattern order and switch

Label: triage
Date: 2026-09-14
Scope: documentation only. No code, test, or commit changes.

## Outputs changed

| File | Change |
| --- | --- |
| `USER_GUIDE.md` | Chinese user guide: seven-tool table, Pattern switch approval + two persisted off-by-default preferences, zero-based order model (duplicates/skip/stop/invalid/unreferenced), switch lifecycle, handoff `pending_review`/`applied` statuses, auto-apply failure semantics, one-Pattern proposal/Undo boundary, manual navigation never rebinds. |
| `sidecar/README.md` | Seven tools and seven translations; `switch_pattern` session pinning and `pending_review` retention release; `get_pattern_order` session-less read; removed "no Pattern selector". |
| `.agents/skills/openmpt-pattern-mcp/SKILL.md` | Router updated: new order/switch references, seven-tool route, status-driven completion, removed only-human-apply and never-claim-applied wording. |
| `.agents/skills/openmpt-pattern-mcp/references/get-pattern-order.md` | New: exact result shape and zero-based order semantics. |
| `.agents/skills/openmpt-pattern-mcp/references/switch-pattern.md` | New: token rules, approval paths, statuses, boundaries. |
| `.agents/skills/openmpt-pattern-mcp/references/session-lifecycle.md` | Switch within a session; status-driven ending. |
| `.agents/skills/openmpt-pattern-mcp/references/handoff-for-review.md` | `applied` vs `pending_review` vs `ok: false` + `pending_review`; no promised automatic success. |
| `.agents/skills/openmpt-pattern-mcp/references/recover-errors.md` | `patternSwitchRejected`, shared `approvalPending`, `candidateExists` switch block, seven-tool boundary, status-limited application claim. |
| `.agents/skills/openmpt-pattern-mcp/references/connect-target.md` | Document switch vs Pattern switch distinction. |
| `.agents/skills/openmpt-pattern-mcp/references/get-pattern-context.md` | `context.pattern` is the binding, not the UI cursor. |
| `.agents/skills/openmpt-pattern-mcp/references/replace-pattern-segment.md` | Approved switch grants whole Pattern; edits block switch. |
| `.agents/skills/openmpt-pattern-mcp/references/release-occupancy.md` | Waiting switch resolution paths. |
| `.agents/skills/openmpt-pattern-mcp/agents/openai.yaml` | Description mentions order read and switch. |

## Ground truth used

- `.scratch/pattern-switch/_spec.md` and `_plan-draft.md`.
- `sidecar/openmpt_mcp.py` (seven-tool `TOOLS`, `update_session_state` retention rules).
- `openmpt-original_ref/mptrack/AIPattern.h/.cpp` (`PatternOrder`, `Switch`, `PerformSwitch`, `ResolveSwitch`, `SwitchRange`, `Configure`, `Dispatch` handoff/`Apply`).
- `openmpt-original_ref/test/mpt_tests_ai_pattern.cpp` and `sidecar/test_native_integration.py` (UI captions `Always allow Pattern switching`, `Always accept submissions`, `Approve Pattern switch`, `Reject Pattern switch`).
- `C:\Users\TouHousandYears\.codex\skills\writing-for-agents\SKILL.md` and `SKILL-MECHANICS.md`.

## Commands run

- Read-only inspection: `git status --short`, `git diff --stat`, `grep`/`sed`/`read` on the files above.
- Link validation (custom Python snippet over touched docs): 18 relative links checked, 0 missing.
- Outdated-claim scan over touched docs: `five-tool`/`five translations` none; `五项能力` none; `no Pattern selector` none; `only the human` none; `never claim ... applied` none; `human owns the review` none; `has not yet been applied` none.

No build, test, or runtime command was run: this slice changes Markdown/YAML only, and the task forbids code/test edits.

## Verified checks

- Every tool documented with the code's actual argument and result names (`get_pattern_order`, `switch_pattern` statuses `unchanged`/`switched`/`pending_approval`, `handoff_for_review` statuses `applied`/`pending_review`).
- Zero-based indices stated for Order entries and Pattern targets; duplicates, `skip`, `stop`, `invalid`, and `unreferenced_patterns` documented from `PatternOrder()` and its native test.
- Token rules: optional only when unoccupied (`occupancyLost` otherwise), rejected switch keeps binding, same target is an authenticated no-op, full target recapture with fresh token.
- Dirty candidate (`candidateExists`) and frozen proposal (`busy`/`occupancyLost`) block switching; no implicit submit/discard.
- One proposal and one Undo per Pattern; no Pattern creation, Order reorder, Sequence switch, or multi-Pattern drafts.
- Manual navigation never rebinds; Sequence/Order/playback untouched by switch.
- Handoff outcomes and failed auto-apply (`ok: false` + `status: pending_review`, proposal retained, occupancy ended, manual review still available); enabling a preference resolves existing waiting switch/proposal.
- Sidecar retention: successful switch pins the published document; `pending_review` handoff (even `ok: false`) releases retention.

## Unverified / residual risk

- The AIService switch-approval UI is implemented concurrently by another writer; captions are pinned by `test_native_integration.py`, but button layout, persisted setting keys, and Patterns display sync were not present in `AIService.cpp` at read time and were not exercised.
- The user guide claim that approval "updates the Patterns page display and clears the previous selection" comes from `_spec.md`; it is not asserted by the C++ unit test (which covers Order/playback immutability only).
- No live OpenMPT run was performed in this offline slice.

## Intentionally historical claims left in place

- `.scratch/pattern-switch/*.log` and `exploration.md` still describe the five-tool baseline; they are dated evidence from the red/green runs and were not edited.
- `sidecar/README.md` "guarded target switching" refers to document-target publication switching, which is unchanged.
- `.agents/skills/.../recover-errors.md` still lists `boundPatternViolation`; it now points to `switch_pattern` as the supported way to change binding.
