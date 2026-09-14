# Cross-Pattern implementation

Label: triage
Baseline: a83f7bc2718cb7cd8730e29d4d630df2866f0ce2 (current branch main).
User confirmed C++ capability, Sidecar protocol and native integration test seams.
2026-09-14 user instruction: whenever visual inspection or UI operations are needed for verification, pause and wait for human verification. Do not run automatic UI clicks, page activation, or screenshots. The native UI tests and _capture_ui.py are authored but must not be executed under this preference. Headless capability/protocol tests and compilation remain allowed.
Main session owns tests; Pi Worker owns production and documentation changes.
Preserve pre-existing worktree changes. Review against the recorded starting HEAD.

Sequential slices:
1. C++ get_pattern_order: current sequence, exact entries, metadata, unreferenced Patterns, no occupancy. Check build-local.ps1 -Test.
2. C++ switch_pattern lifecycle: owner token, manual/automatic approvals, clean candidate boundary, full target capture, cancellation and timeout. Check build-local.ps1 -Test.
3. C++ automatic proposal acceptance: freeze then atomic Apply, default manual, pending failure state, independent preferences. Check build-local.ps1 -Test.
4. Sidecar seven tools and switch token pinning. Check python -m unittest discover -s sidecar -p test_sidecar.py -v.
5. Native service/UI routing: read without view/occupancy, switch approval waiting and connection ownership, independent persisted controls, pending approval/application on enable, safe Patterns display sync. Test real app/pipe with test_native_integration.py and native assertions.
6. User guide, Sidecar README, repository Pattern MCP skill instructions. Inspect exact behavior against docs.
7. Full relevant tests, two-axis Pisub review, fixes, commit only task changes to current branch.

No new Pattern creation, Order rearrangement, Sequence switching, or multi-Pattern drafts. All indices zero-based. Each Apply and Undo touches one Pattern. Manual navigation does not change Agent binding.
