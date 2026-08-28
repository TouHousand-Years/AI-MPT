# Issue blocking relationships

Arrows point from a prerequisite issue to the issue it blocks.

```mermaid
flowchart LR
    I01["01 · Pin upstream baseline<br/>resolved"]
    I02["02 · Inventory application capabilities<br/>resolved"]
    I03["03 · Prototype piano-roll semantics<br/>resolved"]
    I04["04 · Research symbolic score context<br/>resolved"]
    I05["05 · Define shared capability boundary<br/>resolved"]
    I06["06 · Choose sidecar IPC lifecycle<br/>resolved"]
    I07["07 · Choose AI context contract<br/>resolved"]
    I08["08 · Prototype reviewable change workflow<br/>resolved"]
    I09["09 · Choose project distribution model<br/>resolved"]
    I10["10 · Define piano-roll workspace state<br/>open"]
    I11["11 · Choose independent product identity<br/>open"]
    I12["12 · Bootstrap upstream source tree<br/>open"]
    I13["13 · Establish reproducible build and CI<br/>open"]
    I14["14 · Package installer, portable, and sidecar<br/>open"]
    I15["15 · Audit release licenses and source<br/>open"]
    I16["16 · Verify compatibility and lifecycle<br/>open"]
    I17["17 · Publish Developer Preview<br/>open"]
    I18["18 · Harden post-preview distribution<br/>open"]

    I01 --> I02
    I01 --> I09
    I02 --> I05
    I05 --> I06
    I05 --> I08
    I05 --> I10
    I06 --> I08
    I06 --> I09
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
    I03 --> I08
    I04 --> I07
    I07 --> I08

    classDef resolved fill:#d1fae5,stroke:#047857,color:#064e3b
    classDef claimed fill:#fef3c7,stroke:#b45309,color:#78350f
    classDef open fill:#e0f2fe,stroke:#0369a1,color:#0c4a6e

    class I01,I02,I03,I04,I05,I06,I07,I08,I09 resolved
    class I10,I11,I12,I13,I14,I15,I16,I17,I18 open
```

## Legend

- Green: resolved
- Amber: claimed
- Blue: open

## Direct blocking relationships

| Prerequisite | Blocked issue |
| --- | --- |
| 01 | 02, 09, 12 |
| 02 | 05 |
| 03 | 08, 16 |
| 04 | 07 |
| 05 | 06, 08, 10 |
| 06 | 08, 09, 14 |
| 07 | 08 |
| 09 | 10, 11, 12 |
| 11 | 14 |
| 12 | 13, 15 |
| 13 | 14, 15 |
| 14 | 16 |
| 15 | 16 |
| 16 | 17 |
| 17 | 18 |
