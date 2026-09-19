# 核实候选提供商的真实音频能力

Parent: ../map.md
Type: research
Status: resolved
Blocked by: none

## Question

在当前可用账号与正式 API 文档边界内，`qwen3.5-omni-plus`、Gemini 3.1 Pro Preview 与 Gemini 3.8 Flash 的准确模型 ID、原生音频输入方式、支持格式与大小/时长限制、结构化输出能力、请求取消、响应标识、凭据配置、价格和配额分别是什么；哪些差异必须进入提供商无关契约或后续人工对照评测？

## Completion evidence

- 每个候选的材料均来自官方文档或当前账号的安全只读能力探测，并记录查询日期；不记录或暴露凭据。
- 用同一张对照表标出已证实、未证实和账号相关事实，核实模型 ID、输入/输出、限制、取消、响应标识、价格和配额。
- 把域契约必须统一的约束与仅应留在适配器的差异分开，并给出后续人工对照评测的最小输入条件。

## Answer

查询日期：**2026-09-15**。以下只采用提供商官方文档；未读取环境变量、配置文件、控制台或任何凭据，也没有发起会产生费用的模型请求。状态含义：**已证实**＝官方公开资料直接支持；**未证实**＝官方资料没有给出目标语义；**账号相关 / evidence gap**＝必须在用户授权后由启动探测或控制台只读查看确认，本研究不能安全断言。

### 同表对照

| 候选 | 模型 ID、输入与输出 | 格式、大小与时长 | 结构化输出 | 请求取消与响应标识 | 凭据 | 公开价格与配额 | 当前账号 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Qwen3.5 Omni Plus | **已证实**：浮动 ID `qwen3.5-omni-plus`，当前对应快照 `qwen3.5-omni-plus-2026-03-15`；通过 OpenAI-compatible Chat Completions 的 `input_audio` 接受 URL 或 Base64 音频；只支持流式响应，可选仅文本或文本+音频输出。官方示例响应包含 `chatcmpl-...` 的 `id`。[Qwen-Omni](https://www.alibabacloud.com/help/en/model-studio/qwen-omni) | **已证实**：AMR、WAV、3GP、3GPP、AAC、MP3；公开 URL 单文件上限 2 GB，Base64 编码字符串小于 10 MB，音频最长 3 小时；同一请求最多 2048 个 URL 或 250 个 Base64 文件。[Qwen-Omni 音频输入限制](https://www.alibabacloud.com/help/en/model-studio/qwen-omni) | **部分证实**：Qwen3.5-Omni-Plus 支持 `json_object`，保证有效 JSON、**不保证固定字段/类型**；官方 `json_schema` 支持清单不含该模型。因此 Harness 必须自行按 Pydantic schema 校验并把不匹配作为失败，不能把它宣称为提供商严格结构化输出。[Qwen 结构化输出](https://help.aliyun.com/en/model-studio/qwen-structured-output) | **部分证实**：官方说明中断流后，只计服务器收到终止前已生成的输出 token；未找到该 Chat Completions 模型可按响应 ID 调用的服务端取消端点。响应 `id` 已证实，但“中断已被服务端确认”须视为未证实。[Qwen 流式输出](https://www.alibabacloud.com/help/en/model-studio/stream) | **已证实**：`DASHSCOPE_API_KEY`，Bearer 认证；北京与新加坡 key、workspace endpoint 和区域不同，必须与模型部署区域绑定且不得写入任务/日志。[Qwen-Omni 调用示例](https://www.alibabacloud.com/help/en/model-studio/qwen-omni) | **已证实（公开基准）**：新加坡国际区每百万 token：音频输入 $11、以多模态输入生成文本 $8.3；若请求音频输出则音频输出 $44。免费额度 100 万 token，仅新加坡且有效期 90 天。北京公开价分别为 $7.29、$5.5、$29.29。公开限流为 60 RPM、100,000 TPM（输入+输出；新加坡与北京相同）。[价格](https://www.alibabacloud.com/help/en/model-studio/model-pricing)；[限流](https://www.alibabacloud.com/help/en/model-studio/rate-limit) | **账号相关 / evidence gap**：未安全核实账号是否开通、所在 region/workspace、是否可列出该浮动/快照 ID、剩余免费额度、余额和实际提高后的配额。启动检查必须用用户选定区域做只读 model/capability 探测或最小授权探测。 |
| Gemini 3.1 Pro Preview | **已证实**：`gemini-3.1-pro-preview`；输入 Text、Image、Video、Audio、PDF，输出 Text，输入上限 1,048,576 token、输出上限 65,536 token。[模型页](https://ai.google.dev/gemini-api/docs/models/gemini-3.1-pro-preview) | **已证实（Gemini 音频接口公共限制）**：可先经 Files API 上传，或随请求 inline；inline 整个请求小于 20 MB；Files API 每文件 2 GB、每项目 20 GB、保留 48 小时；每 prompt 最长 9.5 小时。支持 WAV、MP3、AIFF、AAC、OGG、FLAC、MPEG、M4A、L16、Opus、ALAW、MULAW、WebM；服务会把多声道合成单声道。[音频理解](https://ai.google.dev/gemini-api/docs/audio)；[Files API](https://ai.google.dev/gemini-api/docs/files) | **已证实**：模型页明确支持 Structured outputs；Interactions API 可用 JSON Schema / Pydantic schema 产生 `application/json`，仍应由 Harness 二次校验。[模型页](https://ai.google.dev/gemini-api/docs/models/gemini-3.1-pro-preview)；[结构化输出](https://ai.google.dev/gemini-api/docs/structured-output) | **已证实但有边界**：Interaction 资源提供唯一 `id`；`POST /v1beta/interactions/{id}/cancel` 只适用于仍在运行的 **background interaction**。同步/普通流请求没有被该端点覆盖，适配器仍须支持本地取消并记录是否获得服务端终态。[Interactions API](https://ai.google.dev/api/interactions-api) | **已证实**：Gemini API key；SDK 推荐从 `GEMINI_API_KEY` 或 `GOOGLE_API_KEY` 读取，二者同时存在时后者优先。凭据只能由进程环境/凭据存储注入。[API key 指南](https://ai.google.dev/gemini-api/docs/api-key) | **已证实（公开基准）**：无免费层；Standard 每百万 token 输入 $2（prompt ≤200k）或 $4（>200k），输出（含 thinking）$12 或 $18。[价格](https://ai.google.dev/gemini-api/docs/pricing) 配额按 project、model、usage tier 变化，preview 通常更严格，实际值只能在 AI Studio 查看。[限流](https://ai.google.dev/gemini-api/docs/rate-limits) | **账号相关 / evidence gap**：未安全核实项目是否能列出 preview ID、付费层级、active limits、余额或 preview 生命周期。必须在启动时调用 models get/list 并把 preview 下线视为 `reviewerCapabilityMismatch` / `providerUnavailable`，不能因文档存在就假定账号可用。 |
| Gemini 3.8 Flash | **已证实**：稳定 ID `gemini-3.8-flash`；输入 Text、Image、Video、Audio、PDF，输出 Text，输入上限 1,048,576 token、输出上限 65,536 token。[模型页](https://ai.google.dev/gemini-api/docs/models/gemini-3.8-flash) | **已证实**：与上行同一 Gemini 音频/Files API 方式和公共限制；尤其 inline 请求小于 20 MB、上传单文件 2 GB、每 prompt 最长 9.5 小时、列出的 13 种 MIME 类型，以及多声道折叠为单声道。[音频理解](https://ai.google.dev/gemini-api/docs/audio)；[Files API](https://ai.google.dev/gemini-api/docs/files) | **已证实**：模型页明确支持 Structured outputs，官方示例以该 ID 配合 JSON Schema/Pydantic；Harness 仍负责最终领域校验。[模型页](https://ai.google.dev/gemini-api/docs/models/gemini-3.8-flash)；[结构化输出](https://ai.google.dev/gemini-api/docs/structured-output) | **已证实但有边界**：同 Gemini Interactions API；唯一 interaction `id` 可记录，按 ID 取消只覆盖运行中的后台 interaction。[Interactions API](https://ai.google.dev/api/interactions-api) | **已证实**：与 Gemini 3.1 Pro 相同的 key 配置和保护规则。[API key 指南](https://ai.google.dev/gemini-api/docs/api-key) | **已证实（公开基准）**：Standard 免费层免费；付费层在 2026-12-31 前每百万 token 输入 $0.75、输出（含 thinking）$3.75，2027-01-01 起为 $1.50 / $7.50。[价格](https://ai.google.dev/gemini-api/docs/pricing) 实际 RPM/TPM/RPD 与容量仍随 project/tier 变化，只能在 AI Studio 看 active limits。[限流](https://ai.google.dev/gemini-api/docs/rate-limits) | **账号相关 / evidence gap**：未安全核实项目是否可列出该 ID、free/paid tier、active limits 或余额；启动时必须只读探测。 |

补充成本口径：Gemini 音频按 32 token/秒计入输入；Qwen3.5-Omni 音频输入按 7 token/秒计入。二者都应由适配器返回提供商 usage 原值，域层只存标准化账单摘要，不能用一套 token 公式跨提供商估算。[Gemini 音频技术细节](https://ai.google.dev/gemini-api/docs/audio)；[Qwen-Omni 计费规则](https://www.alibabacloud.com/help/en/model-studio/qwen-omni)

### 必须进入提供商无关域契约

- **规范化音频与硬上限**：首版共同接受的容器至少可冻结为 WAV；域请求必须携带 MIME、字节数、时长、声道、采样率和 `contentHash`，应用/Harness 的时长与字节上限取三者以及本地策略的最小值，调用者只能收紧。由于 Gemini 会把多声道合成单声道，首版对照评测使用单声道，避免声道处理成为隐藏变量。
- **能力启动检查**：对精确 model ID 做可列出/可调用、audio input、text output、structured-result mode 检查；浮动别名还要记录解析到的快照/版本。能力不足必须 typed failure，禁止退化成“没听音频的文本评审”。
- **Harness 终审 schema**：统一的评审 schema、版本、必填字段与枚举完全属于域层；提供商结构化输出只是优化。Gemini 可下推 JSON Schema，Qwen3.5-Omni-Plus 只能请求 JSON Object，再由 Harness 严格校验并有限重试。
- **取消是任务语义，不是假定的云端保证**：域层统一 `cancelRequested`、`cancelled`、`completedAfterCancel` / 明确失败；适配器记录 `local_abort`、`provider_acknowledged`、`not_supported`。只有收到提供商终态才可声称远端已取消。
- **来源与可追踪性**：`provider`、请求时使用的精确 model ID、可选 resolved snapshot、provider response/interaction ID、输入哈希、提示词版本、schema 版本、usage、开始/结束时间都进入 AI 来源包络；提供商没有或尚未返回 ID 时字段为 `null` 加原因，不能伪造。
- **凭据和配额错误**：域层只表达 provider/region/credential reference（不含秘密）及 `providerUnavailable`、`reviewerCapabilityMismatch`、`rateLimited`、`quotaExceeded`；密钥值、workspace 私密信息、余额只留在安全配置与适配器内部。

### 只应留在适配器的差异

- URL、Base64、Gemini Files API URI 等上传传输形式及临时文件删除/48 小时生命周期；Qwen workspace/region endpoint 与 Gemini project/key 选择。
- Qwen 的 `input_audio` + 强制 streaming、Gemini Interactions content 表示；Qwen `chatcmpl-...` 与 Gemini interaction `id` 的字段路径。
- Qwen JSON Object 提示与本地重试、Gemini JSON Schema 下推；后台 interaction 取消端点与断流取消的不同实现。
- 提供商格式白名单、单请求文件数量、token 换算、价格、RPM/TPM/RPD、重试头和区域价格。这些进入版本化 capability snapshot/诊断，不进入 MCP 的 provider-specific 分支。

### 后续人工对照评测的最小输入条件

1. 固定同一批不可变音频字节：至少 5 个音色（普通钢琴、Bass、鼓、弦/管、合成）各 2–5 秒，以及 2 个具有已知不同问题的乐段各 20–30 秒；统一为**单声道 PCM WAV**，每项 Base64 后小于 10 MB、整个 Gemini inline 请求小于 20 MB，并记录原始 `contentHash`。这样无需公网 URL 或持久上传即可覆盖三个候选。
2. 三者使用同一系统语义、同一任务提示、同一目标领域 schema、同一匿名化范围摘要；Qwen 请求 JSON Object 后本地校验，Gemini 下推同一 JSON Schema。禁止为某模型补充答案性提示。
3. 每个音频×模型至少重复 3 次，保存盲化的结构化结果、自然语言说明、延迟、usage、provider ID、失败/重试和精确 model ID；人工评审者看不到模型名与价格。
4. 评审最少比较：音色 family/role/character 与不确定性；乐段明显问题命中、误报、时间定位、四维完整度；另记 schema 首次通过率、取消终态、成本和延迟。默认模型只能由这组人工对照结果决定。
5. 开测前的人工安全门：用户选择并授权提供商/区域和发送这批有界音频；由用户或受权 UI 确认三个 model ID 在当前账号可列出、active quota 足够。该项目前是明确的 **evidence gap**，未通过时不得进行真实调用。
