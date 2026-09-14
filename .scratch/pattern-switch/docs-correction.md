# Documentation correction: Pattern switch result claims

Label: triage
Scope: Markdown only. No code, test, or commit changes. Offline; no skills or network.

## Corrected claims

Ground truth read in the current working tree: `openmpt-original_ref/mptrack/AIPattern.cpp`,
`AIPattern.h`, `AIService.cpp`, and `sidecar/openmpt_mcp.py`.

1. **Handoff diff is not always returned.** `handoff_for_review` returns the full
   `diff` only on the manual path (`AIPattern.cpp:339`, `status: pending_review`,
   `ok: true`). The automatic path calls `Apply()` (`:338`), which returns only
   `ok` and `status: applied` (`:657`), or a failure carrying
   `status: pending_review` (`:643-644`) with no diff.
   - `references/handoff-for-review.md`: diff guarantee limited to manual
     handoff; verification kept as a precondition and required from the agent's
     own reads since auto-apply returns no diff.
2. **Same-target switch refreshes the idle deadline.** `Switch()` keeps the
   binding and token but resets `m_deadline` when authenticated
   (`AIPattern.cpp:500-508`). The same refresh happens on a rejected switch with
   a retained session (`:592`).
   - `references/switch-pattern.md`: "same token and its deadline are kept" ->
     binding/token unchanged, normal idle retention timeout refreshed.
   - `USER_GUIDE.md`: same-target switch is no longer described as a pure no-op;
     it keeps the binding and refreshes the idle timeout.
3. **`pending_approval` is app-internal, not a tool result.** `Replace` and
   `Switch` emit it (`AIPattern.cpp:442, 529`); `AIService.cpp:921` holds the
   queued request instead of completing it, and the human decision later
   completes it with the final result (`AIService.cpp:1162-1173`). The Sidecar
   blocks reading that reply and returns it unchanged.
   - `references/switch-pattern.md` and `references/replace-pattern-segment.md`:
     waiting call returns the final resolution (`switched`/`unchanged` or the
     replacement result), or a typed rejection; internal marker is never
     returned. A concurrent second call is refused with `approvalPending`.
   - `references/recover-errors.md`: `approvalPending` only answers a concurrent
     second call; the waiting call resolves normally.
   - `sidecar/README.md`: documented the internal hold and final-result return.
   - `USER_GUIDE.md`: the waiting call returns one final result, not a separate
     waiting state.
4. **A session-less `switch_pattern` can establish a session.** `PerformSwitch`
   adopts the target capture, sets `m_retained = true`, and returns the new
   `session` token for authenticated and session-less requests alike
   (`AIPattern.cpp:568-572`); `Switch()` reserves occupancy while a session-less
   request waits (`:521-527`).
   - `SKILL.md` operating step 3 and `references/session-lifecycle.md` now name
     this second acquisition path.
   - `USER_GUIDE.md` states that an approved session-less switch creates a new
     Agent session.
   - `sidecar/README.md`: only a switch result that carries a `session`
     token counts as retained; a session-less `unchanged` does not.
5. **Completion is proven by the ending result, not by a call count.** An ending
   call can fail before the freeze with the token rejected (`occupancyLost` /
   `stale` from `Dispatch`), or after the freeze with `ok: false` +
   `status: pending_review` from `Apply` (`AIPattern.cpp:643`).
   - `SKILL.md` completion paragraph and `references/session-lifecycle.md`
     mandatory ending: require `ok: true` from an ending tool, or handoff's
     `status: pending_review` (which proves freeze and release); a rejected
     ending call before the freeze proves nothing and requires a fresh read.

## Files changed

- `.agents/skills/openmpt-pattern-mcp/SKILL.md`
- `.agents/skills/openmpt-pattern-mcp/references/handoff-for-review.md`
- `.agents/skills/openmpt-pattern-mcp/references/session-lifecycle.md`
- `.agents/skills/openmpt-pattern-mcp/references/switch-pattern.md`
- `.agents/skills/openmpt-pattern-mcp/references/replace-pattern-segment.md`
- `.agents/skills/openmpt-pattern-mcp/references/recover-errors.md`
- `USER_GUIDE.md`
- `sidecar/README.md`

## Commands run and observed results

- Read/AIPattern.cpp, AIPattern.h, AIService.cpp, sidecar/openmpt_mcp.py: confirmed
  the five code behaviors above.
- `grep -rn "pending_approval|deadline|diff"` over the owned docs: the only
  remaining `pending_approval` mentions now describe it as internal/non-result.
- Relative-link check over all 11 skill files, USER_GUIDE.md, and
  sidecar/README.md: 18 links checked, 0 missing.
- `grep` for `exactly one ending` in owned docs: no longer present.
- No build, test, or runtime command was run; the change set is Markdown only.

## Accepted exceptions and residual risk

- `.scratch/pattern-switch/docs-result.md`, `exploration.md`, and `_plan-draft.md`
  still record `pending_approval` as a documented status; they are dated
  historical evidence and are outside the corrected file set.
- `references/switch-pattern.md` retains the UI claim that the Patterns page
  display follows the target and clears the previous selection; this remains
  unverified against a live UI run (unchanged from the prior documentation slice).
- The Sidecar `update_session_state` corner case where a stale local
  `retained_session` flag meets a successful session-less `unchanged` is not
  documented; it does not change the corrected retention rule.
