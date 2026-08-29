# OpenMPT issue blocking relationships

当前快照：`issues` 目录中的 25 个 issue。箭头表示各 issue 文件中声明的直接阻塞关系；节点颜色区分已解决、开放和因当前目的地收缩而关闭的历史票据。

```mermaid
flowchart LR
    I01["01 · Starting Snapshot provenance<br/>resolved"]
    I02["02 · Capability inventory<br/>resolved"]
    I03["03 · Piano Roll semantics<br/>resolved"]
    I04["04 · Symbolic Score Context<br/>resolved"]
    I05["05 · Capability boundary<br/>resolved"]
    I06["06 · Sidecar IPC lifecycle<br/>resolved"]
    I07["07 · AI context contract<br/>resolved"]
    I08["08 · Reviewable AI changes<br/>resolved"]
    I09["09 · Distribution model<br/>closed · out of scope"]
    I10["10 · Piano Roll workspace state<br/>resolved"]
    I11["11 · Product identity<br/>resolved"]
    I12["12 · Available Starting Snapshot<br/>resolved"]
    I13["13 · Reproducible build & CI<br/>closed · out of scope"]
    I14["14 · Installer / portable / sidecar<br/>closed · out of scope"]
    I15["15 · Release license audit<br/>closed · out of scope"]
    I16["16 · Compatibility lifecycle<br/>closed · out of scope"]
    I17["17 · Developer Preview<br/>closed · out of scope"]
    I18["18 · Post-preview maintenance<br/>closed · out of scope"]
    I19["19 · Piano Roll subordinate Tracker UI<br/>open"]
    I20["20 · First useful vertical slice<br/>resolved"]
    I21["21 · Smallest local build loop<br/>resolved"]
    I22["22 · Synchronized Piano Roll slice<br/>open"]
    I23["23 · Minimum AI Pattern capabilities<br/>open"]
    I24["24 · Local MCP round trip<br/>open"]
    I25["25 · Proposal-review-apply loop<br/>open"]

    I12 --> I21
    I20 --> I21
    I03 --> I22
    I10 --> I22
    I20 --> I22
    I21 --> I22
    I05 --> I23
    I20 --> I23
    I06 --> I24
    I21 --> I24
    I23 --> I24
    I08 --> I25
    I22 --> I25
    I23 --> I25
    I24 --> I25
    I03 --> I19
    I10 --> I19
    I22 --> I19
    I25 --> I19

    classDef resolved fill:#d1fae5,stroke:#047857,color:#064e3b
    classDef open fill:#e0f2fe,stroke:#0369a1,color:#0c4a6e
    classDef outscope fill:#f3f4f6,stroke:#6b7280,color:#374151
    class I01,I02,I03,I04,I05,I06,I07,I08,I10,I11,I12 resolved
    class I20,I21 resolved
    class I19,I22,I23,I24,I25 open
    class I09,I13,I14,I15,I16,I17,I18 outscope
```

## 图例

- 绿色：`resolved`
- 蓝色：`open`
- 灰色：因当前目的地收缩而关闭，不属于 Decisions so far

## 直接阻塞关系

| 前置 ticket | 被阻塞 ticket |
| --- | --- |
| Piano Roll projection and editing semantics | Synchronized Piano Roll editing slice；Piano Roll Focus subordinate Tracker UI |
| Shared Application Capability boundary | Minimum AI Pattern capability slice |
| Local MCP sidecar IPC and lifecycle | Local MCP round trip |
| Reviewable AI change workflow | First proposal-review-apply loop |
| Piano Roll workspace state and synchronization | Synchronized Piano Roll editing slice；Piano Roll Focus subordinate Tracker UI |
| Available OpenMPT starting source | Smallest local build-and-run loop |
| First personally useful vertical slice | Smallest local build-and-run loop；Synchronized Piano Roll editing slice；Minimum AI Pattern capability slice |
| Smallest local build-and-run loop | Synchronized Piano Roll editing slice；Local MCP round trip |
| Synchronized Piano Roll editing slice | First proposal-review-apply loop；Piano Roll Focus subordinate Tracker UI |
| Minimum AI Pattern capability slice | Local MCP round trip；First proposal-review-apply loop |
| Local MCP round trip | First proposal-review-apply loop |
| First proposal-review-apply loop | Piano Roll Focus subordinate Tracker UI |

当前开放前沿是 `19`、`22`、`23`、`24`、`25`。其中 `22` 依赖已解决的 `03`、`10`、`20`、`21`；`23` 依赖已解决的 `05`、`20`；`22` 与 `23` 汇入 `24` 和 `25`，最终 `25` 与 `22` 一起阻塞 `19`。`09`、`13`–`18` 虽在元数据中为 `resolved`，但正文明确标记为当前目的地之外关闭，因此以灰色历史节点展示。
