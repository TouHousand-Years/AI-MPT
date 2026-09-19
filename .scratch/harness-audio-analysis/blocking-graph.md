# Harness 音频分析内核：从已接受方案到可实施契约 的阻塞关系图

来源：[`map.md`](map.md)；Map 类型为 `wayfinder:map`。方向约定为「阻塞任务 → 被其阻塞的任务」。

预览：[`blocking-graph.svg`](blocking-graph.svg)。

```mermaid
flowchart LR
    I01["01 核实原生音色捕获接缝<br/>Type: research<br/>Status: resolved"]
    I02["02 原型化有界乐段渲染契约<br/>Type: prototype<br/>Status: open"]
    I03["03 核实候选提供商的真实音频能力<br/>Type: research<br/>Status: resolved"]
    I04["04 决定 Harness 的进程与依赖边界<br/>Type: grilling<br/>Status: open"]
    I05["05 决定统一音频包络与资源上限<br/>Type: grilling<br/>Status: open"]
    I06["06 定义评审结果与来源契约<br/>Type: grilling<br/>Status: open"]
    I07["07 定义试听任务生命周期与临时资源保留<br/>Type: grilling<br/>Status: open"]
    I08["08 定义 MCP 试听能力面<br/>Type: grilling<br/>Status: open"]
    I09["09 原型化人工试听与提交工作流<br/>Type: prototype<br/>Status: open"]
    I10["10 决定项目级音频 Agent 提示词契约<br/>Type: grilling<br/>Status: open"]
    I11["11 定义阶段 0 的验证门与实施交接<br/>Type: grilling<br/>Status: open"]
    I01 --> I05
    I02 --> I05
    I03 --> I05
    I03 --> I06
    I04 --> I07
    I05 --> I07
    I06 --> I07
    I06 --> I08
    I07 --> I08
    I05 --> I09
    I06 --> I09
    I07 --> I09
    I08 --> I09
    I04 --> I10
    I01 --> I11
    I02 --> I11
    I03 --> I11
    I04 --> I11
    I05 --> I11
    I06 --> I11
    I07 --> I11
    I08 --> I11
    I09 --> I11
    I10 --> I11
    linkStyle 0 stroke:#15803d,stroke-width:3px;
    linkStyle 1 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 2 stroke:#15803d,stroke-width:3px;
    linkStyle 3 stroke:#15803d,stroke-width:3px;
    linkStyle 4 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 5 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 6 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 7 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 8 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 9 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 10 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 11 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 12 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 13 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 14 stroke:#15803d,stroke-width:3px;
    linkStyle 15 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 16 stroke:#15803d,stroke-width:3px;
    linkStyle 17 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 18 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 19 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 20 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 21 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 22 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    linkStyle 23 stroke:#c2410c,stroke-width:3px,stroke-dasharray:8 6;
    classDef status_resolved fill:#dcfce7,stroke:#15803d,color:#14532d;
    classDef status_open fill:#f8fafc,stroke:#64748b,color:#334155;
    class I01,I03 status_resolved;
    class I02,I04,I05,I06,I07,I08,I09,I10,I11 status_open;
```

## 当前解读

| 任务 | 类型 | 状态 | 当前仍未解除的上游阻塞 |
| --- | --- | --- | --- |
| [01 核实原生音色捕获接缝](issues/01-verify-native-timbre-capture-seam.md) | `research` | `resolved` | — |
| [02 原型化有界乐段渲染契约](issues/02-prototype-bounded-passage-render-contract.md) | `prototype` | `open` | — |
| [03 核实候选提供商的真实音频能力](issues/03-research-provider-audio-capabilities.md) | `research` | `resolved` | — |
| [04 决定 Harness 的进程与依赖边界](issues/04-choose-harness-process-boundary.md) | `grilling` | `open` | — |
| [05 决定统一音频包络与资源上限](issues/05-choose-unified-audio-envelope.md) | `grilling` | `open` | [02 原型化有界乐段渲染契约](issues/02-prototype-bounded-passage-render-contract.md) |
| [06 定义评审结果与来源契约](issues/06-define-review-and-provenance-contracts.md) | `grilling` | `open` | — |
| [07 定义试听任务生命周期与临时资源保留](issues/07-define-audition-task-lifecycle.md) | `grilling` | `open` | [04 决定 Harness 的进程与依赖边界](issues/04-choose-harness-process-boundary.md), [05 决定统一音频包络与资源上限](issues/05-choose-unified-audio-envelope.md), [06 定义评审结果与来源契约](issues/06-define-review-and-provenance-contracts.md) |
| [08 定义 MCP 试听能力面](issues/08-define-mcp-audition-surface.md) | `grilling` | `open` | [06 定义评审结果与来源契约](issues/06-define-review-and-provenance-contracts.md), [07 定义试听任务生命周期与临时资源保留](issues/07-define-audition-task-lifecycle.md) |
| [09 原型化人工试听与提交工作流](issues/09-prototype-human-audition-workflow.md) | `prototype` | `open` | [05 决定统一音频包络与资源上限](issues/05-choose-unified-audio-envelope.md), [06 定义评审结果与来源契约](issues/06-define-review-and-provenance-contracts.md), [07 定义试听任务生命周期与临时资源保留](issues/07-define-audition-task-lifecycle.md), [08 定义 MCP 试听能力面](issues/08-define-mcp-audition-surface.md) |
| [10 决定项目级音频 Agent 提示词契约](issues/10-choose-project-audio-agent-prompt-contract.md) | `grilling` | `open` | [04 决定 Harness 的进程与依赖边界](issues/04-choose-harness-process-boundary.md) |
| [11 定义阶段 0 的验证门与实施交接](issues/11-define-phase-zero-verification-gate.md) | `grilling` | `open` | [02 原型化有界乐段渲染契约](issues/02-prototype-bounded-passage-render-contract.md), [04 决定 Harness 的进程与依赖边界](issues/04-choose-harness-process-boundary.md), [05 决定统一音频包络与资源上限](issues/05-choose-unified-audio-envelope.md), [06 定义评审结果与来源契约](issues/06-define-review-and-provenance-contracts.md), [07 定义试听任务生命周期与临时资源保留](issues/07-define-audition-task-lifecycle.md), [08 定义 MCP 试听能力面](issues/08-define-mcp-audition-surface.md), [09 原型化人工试听与提交工作流](issues/09-prototype-human-audition-workflow.md), [10 决定项目级音频 Agent 提示词契约](issues/10-choose-project-audio-agent-prompt-contract.md) |

- 节点数：11；依赖边数：24。
- 当前开放且无未解除上游阻塞的任务：[02 原型化有界乐段渲染契约](issues/02-prototype-bounded-passage-render-contract.md), [04 决定 Harness 的进程与依赖边界](issues/04-choose-harness-process-boundary.md), [06 定义评审结果与来源契约](issues/06-define-review-and-provenance-contracts.md)。
- 其中已 claimed 的任务：—。
