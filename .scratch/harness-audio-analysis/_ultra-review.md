# Ultra review of the Harness audio-analysis Wayfinder chart

Status at review time: conditional; the graph was acyclic and destination fidelity was strong, but two blockers required chart amendments before publication.

## Blockers raised

1. Ticket-level completion evidence was absent, so accepted requirements could not be traced to observable acceptance artifacts or checks.
2. The boundary between “原型化有界乐段渲染契约” and deeper tracker-control-flow fog was ambiguous. The ticket needed explicit supported/unsupported behavior, typed failures, state restoration and a planning-only prototype deliverable.

## Amendments made by the main session

- Added `## Completion evidence` to every ticket, including explicit ownership of optional pitch and C-5, native audio ownership, exact range semantics, state restoration, resource limits, review provenance, same-hash human/AI evidence, session-only retention, saved/unsaved project prompts, typed failures, owning-thread dispatch and realtime-callback exclusion.
- Clarified the map fog and the bounded-passage prototype so the first release may explicitly reject unsupported control-flow cases rather than silently truncate them or expand Wayfinder into production renderer implementation.
- Added cross-ticket consistency checks for prompt-read timing and task lifecycle.
- Bounded the final verification ticket to a traceability/integration gate that assembles upstream decisions rather than inventing new semantics.

## Suggestions retained for ticket work

- “决定统一音频包络与资源上限” may draft provider-independent invariants before its blockers close, but final approval continues to wait for both native paths and provider evidence.
- “定义评审结果与来源契约” keeps its conservative provider-research blocker while requiring provider-independent domain semantics.
- Do not split the final verification gate unless resolving upstream tickets reveals genuinely independent decisions.

## Frontier conclusion after amendment

The intended open, unblocked and unclaimed frontier is:

- 核实原生音色捕获接缝
- 原型化有界乐段渲染契约
- 核实候选提供商的真实音频能力
- 决定 Harness 的进程与依赖边界

The original Ultra response did not re-review the amended chart; the main session must validate the literal files and dependency graph before handoff.
