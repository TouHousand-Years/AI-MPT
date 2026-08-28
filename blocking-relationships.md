# OpenMPT issue blocking relationships

当前快照：`issues` 目录中的 19 个 issue。箭头从前置 issue 指向被其阻塞的 issue；节点颜色表示当前状态。

```mermaid
flowchart LR
    I01["01 · Upstream Baseline<br/>resolved"]
    I02["02 · Capability inventory<br/>resolved"]
    I03["03 · Piano Roll semantics<br/>resolved"]
    I04["04 · Symbolic Score Context<br/>resolved"]
    I05["05 · Capability boundary<br/>resolved"]
    I06["06 · Sidecar IPC lifecycle<br/>resolved"]
    I07["07 · AI context contract<br/>resolved"]
    I08["08 · Reviewable AI changes<br/>resolved"]
    I09["09 · Distribution model<br/>resolved"]
    I10["10 · Piano Roll workspace state<br/>resolved"]
    I11["11 · Product identity<br/>resolved"]
    I12["12 · Bootstrap source tree<br/>open"]
    I13["13 · Reproducible build & CI<br/>open"]
    I14["14 · Installer / portable / sidecar<br/>open"]
    I15["15 · Release license audit<br/>open"]
    I16["16 · Compatibility lifecycle<br/>open"]
    I17["17 · Developer Preview<br/>open"]
    I18["18 · Post-preview maintenance<br/>open"]
    I19["19 · Piano Roll subordinate Tracker UI<br/>open"]

    I01 --> I02
    I01 --> I09
    I02 --> I05
    I05 --> I06
    I05 --> I08
    I03 --> I08
    I04 --> I07
    I07 --> I08
    I05 --> I10
    I09 --> I10
    I09 --> I11
    I01 --> I12
    I09 --> I12
    I12 --> I13
    I06 --> I14
    I11 --> I14
    I13 --> I14
    I12 --> I15
    I13 --> I15
    I03 --> I16
    I14 --> I16
    I15 --> I16
    I16 --> I17
    I17 --> I18
    I03 --> I19
    I10 --> I19

    classDef resolved fill:#d1fae5,stroke:#047857,color:#064e3b
    classDef open fill:#e0f2fe,stroke:#0369a1,color:#0c4a6e
    class I01,I02,I03,I04,I05,I06,I07,I08,I09,I10,I11 resolved
    class I12,I13,I14,I15,I16,I17,I18,I19 open
```

## 图例

- 绿色：`resolved`
- 蓝色：`open`

## 直接阻塞关系

| 前置 issue | 被阻塞 issue |
| --- | --- |
| 01 | 02、09、12 |
| 02 | 05 |
| 03 | 08、16、19 |
| 04 | 07 |
| 05 | 06、08、10 |
| 06 | 14 |
| 07 | 08 |
| 09 | 10、11、12 |
| 10 | 19 |
| 11 | 14 |
| 12 | 13、15 |
| 13 | 14、15 |
| 14 | 16 |
| 15 | 16 |
| 16 | 17 |
| 17 | 18 |

当前未解决的开放链路最终汇聚到 `16 → 17 → 18`，而 `19` 是由 `03` 与 `10` 汇聚的 Piano Roll UI 支线。
