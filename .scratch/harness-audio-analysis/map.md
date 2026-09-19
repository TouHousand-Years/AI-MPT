# Harness 音频分析内核：从已接受方案到可实施契约

Label: wayfinder:map
Status: open

## Destination

形成一份可直接交给实现阶段的 Harness 音频分析内核契约：原生音频采集与范围渲染边界、统一音频包络、PydanticAI/Sidecar 部署边界、评审与任务生命周期、MCP 工具面、人工试听界面、项目提示词规则和阶段 0 验证门均已决定，并且每项决定都有可检查的验收依据。

## Notes

- 本地图细化已接受的[“添加 Harness 音频分析内核”方案](../ai-collaborative-openmpt/issues/36-add-harness-audio-analysis-core.md)，不重新讨论 PydanticAI 基底、两类试听能力、人工与 AI 同源音频、修订绑定、临时会话数据或“评审不得修改音乐”等已定方向。
- 领域词汇以仓库根目录 `CONTEXT.md` 为准：Tracker Pattern、Piano Roll、Score Context、Document Revision、Audio Preview、Mini Audio Reviewer、Musical Range、Application Capability 与 MCP Sidecar 均沿用既有含义。
- 默认只做规划；每个子票解决一项决策或为决策制造证据，不交付生产实现。
- `research` 票使用 `research`；`prototype` 票使用 `prototype-pisub`；`grilling` 票使用 `grilling-pisub` 与 `domain-modeling`。
- 原生 C++/MFC 侧拥有文档、播放与离线渲染；Python 侧不得重写 Tracker 音频语义。复用现有 owning-thread、attach、Document Revision 和 typed-failure 边界。
- 人工与多模态 AI 必须消费相同 `audioContentHash` 的音频；来源由实际入口生成，不允许 MCP 调用方伪造 `human`。
- 当前 Sidecar 是 Python 标准库单进程且只暴露 Pattern 工具；引入 PydanticAI 的依赖和监督边界必须显式决定。
- 本地 Markdown 跟踪器使用 `Blocked by:` 表达依赖；箭头方向统一为“阻塞票 → 被阻塞票”。

## Decisions so far

<!-- Resolved child-ticket pointers are appended here. -->

- [核实原生音色捕获接缝](issues/01-verify-native-timbre-capture-seam.md)：由文档所有线程绑定 Document Revision 并创建隔离的原生非实时渲染任务，实时试听与捕获共享 preview-note 触发/释放语义；以规范 PCM/WAV 的 SHA-256 标识人工与 AI 共用产物，插件与外部 MIDI 首版明确不支持。
- [核实候选提供商的真实音频能力](issues/03-research-provider-audio-capabilities.md)：三候选均支持真实音频输入，但结构化输出和远端取消能力不同；共同评测可使用有界单声道 WAV，域层必须自行严格校验与追踪取消，账号可用性和实际额度保留为启动探测缺口。

## Not yet specified

- “原型化有界乐段渲染契约”必须先决定首版支持的范围边界、控制流遇到边界时的行为、状态恢复，以及不支持情形的 typed failure；只有在这条显式边界之外、更深的 Pattern break、jump、loop、delay 与插件尾音语义是否值得扩展，才留在雾区。
- 当统一音频包络和真实提供商能力被验证后，是否值得向 Mini Audio Reviewer 额外提供 Score Context 或 Piano Roll 图像，以及这些可选证据如何影响成本和判断质量。
- 当人工试听原型确定可用交互后，是否需要更细的多人身份、评论编辑历史或可访问性规则；首版仍以本机单用户为边界。
- 实际实现和固定夹具可运行后，哪一个候选云端模型应成为默认 Reviewer；模型选择必须来自同一批音色和乐段的人工对照结果。

## Out of scope

- 在本地图中实现、构建或发布 Harness、原生渲染适配器、MCP 工具或 UI。
- 重新选择 Harness 框架，或把试听内核扩展为具有长期记忆、复杂工具循环和多 Agent 协作的通用 Agent。
- 本地音频理解模型、长期评审数据库、跨项目记忆或云端多人协作。
- 让评审结果直接修改 Tracker Pattern、生成 Change Proposal、自动 Apply，或取代 revision-bound Score Context。
- 首版强制生成完整 Context Package、全项目音频或 Piano Roll 图像。
- 复制 OpenMPT 播放/混音语义到 Python，或重做现有 MCP 连接、占用与 Proposal 生命周期。
- 在实现提供商适配器和固定评测夹具之前提前指定默认云端模型。
