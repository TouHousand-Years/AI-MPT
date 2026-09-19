# 定义评审结果与来源契约

Parent: ../map.md
Type: grilling
Status: open
Blocked by: 03

## Question

怎样把已接受的“清晰音色直答、困难音色三维描述”和“明显问题优先、否则四维乐段评审”冻结为版本化结果 schema，同时统一 `human` / `multimodal_ai` 来源包络、置信度与不确定性、音频时间/Pattern 行定位、模型响应标识、输入哈希、Document Revision 与 stale 语义，并保证听感推断永远不会伪装成 Score Context 事实？

## Completion evidence

- 提供版本化 timbre、passage 和 provenance schema，以及清晰/困难音色、明显问题/无明显问题/不确定乐段的有效样例与反例。
- 域结果语义不得依赖某个提供商的结构化输出特性；单独记录适配器能力映射和校验失败。
- 自动可检验地要求 Document Revision、同一 `audioContentHash`、来源入口、输入哈希和 AI 模型响应标识，并保证人工与 AI 意见并列而不覆盖。
- stale、无定位、低置信度和与 Score Context 冲突时的表达明确；任何结果都不包含音乐修改或 Apply 能力。
