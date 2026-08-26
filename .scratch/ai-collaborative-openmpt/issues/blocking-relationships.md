# Issue blocking relationships

Arrows point from a prerequisite issue to the issue it blocks.

```mermaid
flowchart LR
    I01["01 · Pin upstream baseline<br/>resolved"]
    I02["02 · Inventory application capabilities<br/>open"]
    I03["03 · Prototype piano-roll semantics<br/>claimed"]
    I04["04 · Research symbolic score context<br/>resolved"]
    I05["05 · Define shared capability boundary<br/>open"]
    I06["06 · Choose sidecar IPC lifecycle<br/>open"]
    I07["07 · Choose AI context contract<br/>open"]
    I08["08 · Prototype reviewable change workflow<br/>open"]
    I09["09 · Choose project distribution model<br/>open"]

    I01 --> I02
    I01 --> I09
    I02 --> I05
    I05 --> I06
    I05 --> I08
    I03 --> I08
    I04 --> I07
    I07 --> I08

    classDef resolved fill:#d1fae5,stroke:#047857,color:#064e3b
    classDef claimed fill:#fef3c7,stroke:#b45309,color:#78350f
    classDef open fill:#e0f2fe,stroke:#0369a1,color:#0c4a6e

    class I01,I04 resolved
    class I03 claimed
    class I02,I05,I06,I07,I08,I09 open
```

## Legend

- Green: resolved
- Amber: claimed
- Blue: open

## Direct blocking relationships

| Prerequisite | Blocked issue |
| --- | --- |
| 01 | 02, 09 |
| 02 | 05 |
| 03 | 08 |
| 04 | 07 |
| 05 | 06, 08 |
| 07 | 08 |

