# Issue 26 implementation progress

Date: 2026-09-07
Status: incomplete — native build prerequisites unavailable
Label: triage

The owner requested implementation and a pause **before manual functional
verification**. No manual verification, playback, Save As/reopen or keep-verdict
has been performed. Issue 26 and issues 27, 29–30 and 32–34 remain open;
issue 31 was resolved on 2026-09-08 (see the update below).

## Delivered in this attempt

- `sidecar/`: stdio translator for exactly the five Pattern tools, explicit
  instance/document attachment and length-prefixed local named-pipe transport.
- Process-boundary tests against a scripted Windows pipe peer. These establish
  translation behavior only; no native facade or real application endpoint is
  present.
- Sidecar README records the proposed app envelope and the integration work
  still required. At the time of this attempt issue 31 was **not** accepted or
  resolved; it was resolved on 2026-09-08 (see the update below).

Verification: `python -m unittest discover -s sidecar -v` — 10 tests passed;
`python -m py_compile sidecar/openmpt_mcp.py sidecar/test_sidecar.py` passed.
No native test suite could run because the native build is blocked below.

## Standards review

Independent code-review standards agent found no documented AGENTS.md
violations or actionable baseline smells. It found two protocol robustness
defects: lone-surrogate request IDs could crash serialization, and malformed
application failure envelopes were forwarded without validation. Both were
fixed with failing-then-passing process regression tests. Targeted re-review
confirmed both fixes and reported no remaining findings.

## Spec review

Independent code-review spec agent found one consolidated partial-delivery
finding: the translator does not supply issue 26's end-to-end working loop or
issue 31's connection to the real app endpoint. This was unresolved at review
time; the issue-31 connection was resolved on 2026-09-08 (see the update
below), while issue 26's end-to-end working loop remains open. No scope creep
or demonstrated semantic violation was found in the delivered translator.

Review result: Standards 0 remaining findings; Spec 1 remaining finding
(incomplete native implementation and integration; the issue-31 portion of
this finding was resolved on 2026-09-08).

## Build prerequisite blocker

The task-21 command references VS 2022 BuildTools, which is absent on this host.
`vswhere` instead reports VS Build Tools 2026 at
`C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools`, MSVC 14.51,
and Windows SDK 10.0.26100.0. MFC/ATL and Spectre libraries are missing.
The Debug x64 build with `/p:PlatformToolset=v145` stops at `MSB8040` before
compiling application code. The original v143 project settings were retained.

An attempt to add the v143 prerequisites through the installed VS Installer
returned code 5007 (installer not elevated). A subsequent UAC launch returned
“operation canceled by the user.” No further elevation attempt was made, and
no prerequisite installation was confirmed.

Required installer components, verified in the local VS catalog:

- `Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64`
- `Microsoft.VisualStudio.Component.VC.14.44.17.14.x86.x64.Spectre`
- `Microsoft.VisualStudio.Component.VC.14.44.17.14.MFC`
- `Microsoft.VisualStudio.Component.VC.14.44.17.14.MFC.Spectre`
- `Microsoft.VisualStudio.Component.VC.14.44.17.14.ATL.Spectre`

## Resume

After the owner installs or authorizes installation of the prerequisites,
verify the native build with the installed MSBuild path and v143. Implement
the capability and review seams with their agreed tests, owning-thread pipe
broker, occupancy write guards and AI/MCP UI. Maintain the existing Piano Roll
pane in the development line. Then connect the Sidecar to the real app, review
and compile the complete change, and prepare the owner checklist. Pause before
the owner's functional acceptance; do not mark the spec complete without it.

## Update 2026-09-08: issue 31 resolved

Issue 31 was resolved and all four of its acceptance criteria were met on
this date. The canonical detailed record — the AC4 disconnect-notice fix in
`AIService.cpp`, the MCP Inspector real-client verification, and the full
verification results — is the 2026-09-08 work log in
`.scratch/ai-collaborative-openmpt/issues/31-mcp-sidecar-stdio-translator.md`.
Result summary: `build-local.ps1` PASS; native suite 3 tests OK; sidecar
discover suite 25 OK with 3 opt-in tests skipped in the non-native run;
official Inspector real call PASS. Issue 26 itself remains open, paused
before the owner's manual functional verification.
