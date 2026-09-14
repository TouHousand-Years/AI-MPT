# OpenMPT for AI 用户操作指南

本指南面向使用本项目构建版 OpenMPT 的音乐创作者，介绍当前已经可用的两条工作流：在钢琴卷帘中直接编辑 Tracker Pattern，以及通过 Codex Agent 生成候选修改并由用户审核。

当前版本仍以 **Tracker Pattern** 作为唯一的乐曲数据。钢琴卷帘是它的图形化编辑视图；Agent 只在私有候选中生成 Pattern 修改提案，默认必须由用户在审核区应用。只有用户显式开启持久化选项 **总是接受提交** 后，冻结的提案才会在通过修订检查、单元格验证和 Undo 准备后自动应用；自动应用失败时文档不变，提案保留待人工处理。

## Pattern 切换简介

Pattern 切换分为“人工视图切换”和“Agent 绑定切换”两件不同的事：你在 Patterns 页或钢琴卷帘中选择另一个 Pattern，只会改变自己的查看位置；Agent 仍绑定原 Pattern。只有 Agent 调用 `switch_pattern` 并获得批准后，编辑目标才会改变。

一次 Agent 会话只绑定一个 Pattern。切换时使用 Sequence 中的零基 Pattern 编号；批准后，目标 Pattern 会重新捕获完整基线，并返回新的会话令牌，旧令牌失效。当前 Pattern 重复出现在多个 Order 位置时，它们仍指向同一个 Pattern，不会产生不同副本。

如果要连续编辑多个 Pattern，应按以下顺序操作：先完成并审核或取消当前 Pattern 的候选修改，再申请切换；确认返回 `switched` 后重新读取目标 Pattern，完成核验后再提交。每份提案和每次 Undo 都只作用于一个 Pattern，不能把多个 Pattern 合并为一份草稿。

## 一、钢琴卷帘窗口

### 1. 打开与认识界面

打开一个模块文件后，在文档控制区选择 **Piano Roll** 页签。

上方控制区包含：

| 控件 | 用途 |
| --- | --- |
| `Pattern` | 选择当前显示和编辑的 Pattern。它与传统 Patterns 页使用同一个 Pattern 编号。 |
| `Instrument` | 选择新建音符使用的乐器；无乐器模块中则选择 Sample。 |
| `Snap` | 开启或关闭行吸附。 |
| `1 / 2 / 4 / 8` | 设置吸附的行数，也决定双击新建音符的默认长度。 |
| `Split by instrument` | 按乐器和音符重叠关系重新整理通道。该操作可能影响整个文档中的多个 Pattern。 |
| `Undo / Redo` | 撤销或重做钢琴卷帘产生的 Pattern 修改。 |
| `Play / Stop` | 播放当前 Pattern 或停止播放。 |
| `Follow Song` | 播放时让横向视图跟随当前播放行。 |

画布横向表示 Pattern 行和时间，纵向表示音高。顶部标尺显示行号与节拍，小节和拍线使用不同强度显示；左侧钢琴键盘和顶部标尺在滚动时保持可见。音符颜色按照乐器区分，较宽的音符会显示乐器名称以及已有的音量、效果信息。

### 2. 新建与试听音符

1. 在 `Instrument` 中选择一个已有乐器或 Sample。
2. 根据需要开启 `Snap` 并选择 1、2、4 或 8 行。
3. 在空白网格中双击，即可在对应行和音高创建音符。

新音符默认从吸附后的行开始；开启吸附时，其初始长度等于吸附行数，否则长度为 1 行。

试听方式：

- 单击左侧钢琴键可以试听当前乐器对应的音高。
- 聚焦画布后，可使用 `Z S X D C V G B H N J M` 这一排键试听从 C-5 开始的十二个半音。
- 松开按键或鼠标后停止本次试听。键盘试听当前只负责预听，不会录入 Pattern。

### 3. 选择音符

- 单击一个音符：只选择该音符。
- `Ctrl` + 单击：把音符加入选择，或从选择中移除。
- 在空白区域按住 `Shift` 拖出选择框：框选相交的音符。
- `Ctrl` + `Shift` 拖框：把框内音符追加到现有选择。
- `Ctrl+A`：选择当前 Piano Roll 投影中的全部音符。

选中的音符会显示高对比度双层边框。切换 Pattern 后，当前选择会被清除。

### 4. 移动、移调和调整长度

- 拖动已选择的音符主体：同时改变时间位置和音高；多选时整体移动。
- 拖动音符左边缘：改变起始行，同时保持结束位置。
- 拖动音符右边缘：改变结束行。
- `←` / `→`：将选中音符移动 1 行，不受 Snap 设置影响。
- `↑` / `↓`：将选中音符升高或降低 1 个半音。

鼠标拖动的位置会应用当前 Snap 设置。操作超出 Pattern 行范围或模块格式支持的音域时，修改会被拒绝并在状态栏显示原因。

只有支持明确 Note Off 事件的模块格式才能可靠写入音符长度。如果当前格式不支持，画布顶部会显示原因，左右边缘调整长度也不可用。

### 5. 删除、复制和粘贴

- 右键单击音符：删除该音符；如果它属于多选，则删除整组选择。
- `Delete` 或 `Backspace`：删除全部选中音符。
- `Ctrl+C`：复制选择。
- `Ctrl+X`：剪切选择。
- `Ctrl+V`：以当前画布光标的行和音高为基准粘贴。

钢琴卷帘使用自己的内部剪贴板，不与传统 Pattern 编辑器或 Windows 系统剪贴板共享格式。复制内容保存相对时间、音高、通道、乐器、音量和长度；粘贴后仍会按照当前的通道路由规则放置。

### 6. 滚动与缩放

- 鼠标滚轮和滚动条：浏览画布。
- `Ctrl` + 滚轮：改变每行的横向宽度，也就是时间轴缩放。
- `Shift` + 滚轮：改变琴键和音高行的纵向高度。
- 单击顶部标尺：把粘贴和操作光标移动到对应行。

开启 `Follow Song` 后，播放到当前 Pattern 时，视图会自动把播放行保持在画面中。需要自由浏览其他位置时可暂时关闭它。

### 7. 通道路由与 Tracker 数据

钢琴卷帘显示的是 Pattern 中的 pitched notes。音符长度根据同一通道后续的音符、Note Off、Note Cut、Note Fade 或相关终止效果推导。

为了让同一乐器的重叠音符能够同时发声，钢琴卷帘使用以下规则写回 Tracker：

1. 按乐器建立通道组。
2. 同一乐器的非重叠音符优先复用最低可用通道。
3. 重叠音符落到额外层；必要时可增加通道。
4. 在格式支持时，为音符结束位置写入 Note Off。

这套重新分配规则会在钢琴卷帘编辑时保持一致，并可能整理文档中其他 Pattern 的音符通道。`Split by instrument` 会显式对整个文档执行同样的整理。操作会作为一个整体进入 OpenMPT 原生 Undo；如果无法完整准备 Undo、超过格式最大通道数，或会覆盖无法安全表达的 Tracker 专用数据，则整个操作失败且不写入部分结果。

效果命令、特殊音符和非普通音量命令仍属于 Tracker 专用数据。钢琴卷帘会尽量保留它们；当它们与目标音符位置冲突时，会拒绝修改并提示原因。遇到这种情况，请回到 **Patterns** 页检查相应单元格。

### 8. 与 Agent 会话同时使用

当 Agent 持有编辑会话时，钢琴卷帘会显示只读提示。此时仍可滚动、查看、试听和播放，但写操作被阻止。完成、取消或强制释放 Agent 会话后才能继续人工编辑。

## 二、Agent 协作

### 1. 首次配置 Codex

当前 MCP Sidecar 需要 Windows 和 Python 3.10 或更高版本。推荐只配置一次自动目标模式。

在用户级 `~/.codex/config.toml`，或受信任项目的 `.codex/config.toml` 中加入：

```toml
[mcp_servers.openmpt]
command = "python"
args = ["C:/Users/qnhxx/Documents/AI-Projects/Code/OpenMPT-for-AI/sidecar/openmpt_mcp.py", "--auto-target"]
required = false
startup_timeout_sec = 10
tool_timeout_sec = 300
```

也可以执行一次：

```powershell
codex mcp add openmpt -- python C:/Users/qnhxx/Documents/AI-Projects/Code/OpenMPT-for-AI/sidecar/openmpt_mcp.py --auto-target
```

配置后重启一次 Codex。以后切换 OpenMPT 文档时，只需在 OpenMPT 中重新连接目标，不需要再次修改 Codex 配置。

### 2. 准备 Agent 的目标与范围

每次开始协作前：

1. 打开需要处理的模块文件。
2. 进入传统 **Patterns** 页，选择目标 Pattern。
3. 如果只希望 Agent 修改一部分内容，在 Patterns 页框选对应行和通道。
4. 打开 **AI / MCP** 页。
5. 确认上方状态为 `Status: MCP ready`。
6. 单击 `Connect active doc to Codex`。

Agent 会在第一次读取时绑定 **Patterns 页当前 Pattern 与选区**。钢琴卷帘中的音符选择不会成为 Agent 的授权范围。仅仅切换窗口焦点也不会自动改变 Codex 目标；最后一次单击连接按钮发布的文档才是当前目标。

之后要让 Agent 编辑同一文档中的另一个 Pattern，不必重新连接：让 Agent 申请切换并在 AI / MCP 页批准即可。人工在 Patterns 页或钢琴卷帘中导航 Pattern 不会改变 Agent 的绑定目标。

连接按钮被拒绝时，先检查：

- `Enable MCP` 是否开启，以及状态是否已经变为 ready。
- 当前是否存在活动文档。
- 文档是否至少打开过 Patterns 页。
- 是否还有未结束的 Agent 会话或等待审核的提案。

### 3. AI / MCP 页面

该页面分为上下两部分。

上部用于连接和设置：

| 控件 | 用途 |
| --- | --- |
| `Enable MCP` | 启动或停止本地 MCP 服务。默认开启。 |
| `Always approve range expansion` | 自动批准 Agent 超出初始选区的编辑范围。默认关闭。 |
| `Always allow Pattern switching` | 自动批准后续合法的 Pattern 切换请求。默认关闭，与范围扩展设置相互独立；开启时若已有切换请求在等待，会立即尝试批准。 |
| `Always accept submissions` | 提交冻结后立即尝试应用。默认关闭，与上述两项相互独立；开启时若已有待审核提案，会立即尝试应用一次。 |
| 秒数输入框 | 设置空闲会话超时，范围 1–3600 秒，默认 300 秒。 |
| `Save settings (seconds)` | 保存上述开关和超时设置。 |
| `Connect active doc to Codex` | 把当前活动文档明确发布为 Codex 目标。 |
| 状态文本区 | 显示服务状态、Pipe、实例 ID、目标文件和所有打开文档。 |

下部用于会话和提案审核：

| 控件 | 用途 |
| --- | --- |
| `RELEASE AI NOW` | 紧急结束当前 Agent 占用；进行中的调用会收到会话丢失结果。 |
| `Approve expansion` | 批准 Agent 超出初始选区的本次范围请求。 |
| `Decline expansion` | 拒绝本次范围扩展。 |
| `Approve Pattern switch` | 批准等待中的 Pattern 切换请求。 |
| `Reject Pattern switch` | 拒绝等待中的 Pattern 切换请求，保留原绑定。 |
| `Apply whole proposal` | 把完整提案作为一次原子修改应用到文档。 |
| `Reject whole proposal` | 丢弃完整提案。 |
| 变更列表 | 按行、通道显示原值、提案值和文档当前值。 |
| 图形证据区 | 并排显示 Baseline、Proposal 和 Current document 的音高分布。 |

部分接受目前不可用：一次只能应用或拒绝整份提案。每份提案和每次 Pattern Undo 都只涉及一个 Pattern；当前不支持切换 Sequence 或多 Pattern 草稿。Agent 可以在批准 Pattern 切换时创建缺失的 Pattern，也可以在持有会话时重排当前 Sequence 的 Order。

### 4. 向 Agent 提出任务

连接完成后，可在同一个 Codex 任务中直接描述音乐目标。尽量说明目标声部、行范围、乐器和希望保留的内容，例如：

```text
读取当前 Pattern，为第 2、3 通道补写两声部和声。保持第 1 通道旋律和第 4 通道贝斯不变，完成后交给我审核。
```

```text
分析当前 Pattern 的前 32 行，把选中通道的旋律整体提高一个八度；保留所有效果和音量信息，然后提交提案。
```

```text
只分析当前选区并说明节奏、调性和声部关系，不要修改。
```

当前 Pattern MCP 为 Agent 提供八项能力。Order 条目索引和 Pattern 编号都从 0 开始：

| 能力 | 用户可见含义 |
| --- | --- |
| 读取 Pattern 顺序 | 列出当前 Sequence 的每个 Order 条目，保留重复引用、跳过、停止与无效引用，并列出未被引用的有效 Pattern。只读，不申请占用，也不要求打开 Patterns 页。 |
| 重排 Pattern 顺序 | 在持有会话时提交当前 Order 下标的完整排列。每个原下标必须恰好出现一次，因此重复 Pattern、`+++`、`---` 和无效条目都会原样移动，不会被增删或改写；操作立即应用并保留当前 Pattern 绑定和令牌。 |
| 切换绑定 Pattern | Agent 申请把编辑目标切换到指定编号的 Pattern。已持有会话时必须携带当前令牌；没有会话时可直接申请，批准后会建立新的 Agent 会话。批准后目标 Pattern 全量重新捕获为基线并返回新令牌，全部行和通道获得授权。若格式范围内的目标不存在且 Order 尚有容量，批准会先按源 Pattern 行数创建目标，并追加到当前 Order 的有效末尾。 |
| 读取 Pattern 上下文 | 获取当前候选或原始基线、节拍、格式、乐器、稀疏单元格，以及当前格式可写的音量/效果命令编号和参数范围。 |
| 替换单通道连续片段 | 在私有候选中累积修改，不直接写入文档；可创建、更改或清除当前模块格式支持的全部音量列与效果列命令。 |
| 提交审核 | 冻结完整候选并释放 Agent 占用。默认转入人工审核；开启“总是接受提交”后立即尝试原子应用。 |
| 取消会话 | 丢弃整个候选。 |
| 释放只读占用 | 结束没有修改的分析会话。 |

### 5. 会话占用与范围审批

Agent 开始编辑后，下部状态会显示 `AI OCCUPIED`。此时：

- 可以导航 Pattern、滚动画面、播放和试听。
- Patterns 页和钢琴卷帘的写操作被阻止。
- 每次成功读取或写入都会刷新空闲超时。
- 如果文档的相关 Pattern、乐器、Sample 或节拍依赖在会话外发生变化，旧候选会被判定为 stale，Agent 必须重新读取。

如果 Agent 请求修改初始选区之外的行或通道，下部会显示请求范围和当前授权范围。审批等待期间超时计时暂停；等待中的调用会在批准或拒绝后返回最终结果（成功的替换结果或 `rangeRejected` 等类型化失败），不会单独返回等待状态：

- 范围合理时单击 `Approve expansion`，原调用会继续。
- 希望严格限制在原选区时单击 `Decline expansion`。
- 经常处理整段 Pattern 且无需逐次确认时，可开启 `Always approve range expansion`；该设置会保存到后续会话。

如果 Agent 申请切换到另一个 Pattern，下部会显示源 Pattern、目标编号与名称，以及批准后获得的整 Pattern（全部行、全部通道）授权范围：

- 单击 `Approve Pattern switch` 后，目标会重新捕获为全新基线并返回新令牌；原绑定与旧令牌失效。
- 单击 `Reject Pattern switch` 保留原绑定；请求来自已有会话的 Agent 时令牌和候选都不变并刷新空闲超时，否则释放等待中的保留。
- 已开启 `Always allow Pattern switching` 时，合法请求自动批准；开启该设置时若已有请求在等待，会立即尝试批准。
- 等待中的切换请求同样暂停空闲超时；强制释放、断连或关闭文档会终止等待，批准时还会重新验证目标与会话。

有未提交的候选修改或待审核提案时，切换会被拒绝，需要先应用、拒绝或取消当前工作。切换到既有 Pattern 不修改 Sequence、Order 内容或播放位置；创建缺失目标时会追加一个 Order 引用。重复的 Order 引用仍然编辑同一个 Pattern。人工在 Patterns 页或钢琴卷帘中导航不会改变 Agent 的绑定目标。

如果 Agent 长时间无响应或需要立即恢复人工编辑，可使用 `RELEASE AI NOW`。该操作用于解除活动会话；若已经生成待审核提案，应使用 `Apply whole proposal` 或 `Reject whole proposal` 明确处理。

### 6. 审核并应用提案

Agent 完成候选后，会把整份修改冻结为单个 Pattern 的提案并释放写占用。随后看返回状态：

- `status: pending_review`（默认）：提案等待人工审核，文档内容尚未改变。
- `status: applied`：仅在已开启“总是接受提交”且提案通过修订检查、单元格验证与 Undo 准备时返回，此时文档已经原子应用修改。

自动应用失败会返回 `ok: false` 且状态仍为 `pending_review`：文档不变，提案保留给人工处理，Agent 占用已经结束。开启“总是接受提交”时，如果已经存在待审核提案，会立即尝试应用一次；失败不会静默重试。无论自动还是手动，每次应用或拒绝都只涉及当前提案对应的那一个 Pattern，一次 Undo 也只撤销该 Pattern。

审核步骤：

1. 查看状态中的 `Proposal current` 或 `Proposal stale`。
2. 在变更列表中逐项检查行、通道、修改前后音符和当前文档值。
3. 单击列表项可把传统 Patterns 页的光标定位到相应 Pattern 单元格；切回 Patterns 页即可核对 Tracker 原始数据。
4. 查看下方 Baseline、Proposal、Current document 三栏图形证据。
5. 整份提案正确时单击 `Apply whole proposal`；否则单击 `Reject whole proposal`。

应用操作会再次检查文档修订和所有候选单元格，准备 OpenMPT 原生 Undo 后再整体写入。任何检查或 Undo 准备失败都会保持文档不变；自动应用失败后，人工仍可用 `Apply whole proposal` 或 `Reject whole proposal` 处理保留的提案。应用成功后仍建议播放检查；如听感不符合预期，可使用 OpenMPT 的 Undo 撤销。

如果提案状态为 stale，说明文档已发生相关变化。不要直接套用旧方案；拒绝提案，重新选择范围并让 Agent 基于最新内容生成新提案。

### 7. 切换文档或 Pattern

- 切换 Pattern：不必先结束会话。让 Agent 申请 `switch_pattern`，再在 AI / MCP 页批准。已持有会话时 Agent 必须携带当前令牌；没有会话时批准后会建立新的 Agent 会话。批准后目标 Pattern 全量重新捕获为基线并返回新令牌，旧令牌失效。切换到当前 Pattern 只保持原绑定并刷新空闲超时；格式范围内的缺失 Pattern 会在批准时创建并追加到 Order。
- 人工导航：在 Patterns 页或钢琴卷帘中切换 Pattern 只改变你的视图，不会改变 Agent 的绑定目标；只有已批准的切换才会改变它。
- 切换到既有 Pattern 不修改 Sequence、Order 内容或播放位置；创建缺失目标时会追加一个 Order 引用。重复的 Order 引用仍指向同一个 Pattern。
- 切换文档：完成或取消当前工作后，激活新文档，并再次单击 `Connect active doc to Codex`。
- 多个 OpenMPT 实例共享同一个当前用户目标文件；最后一次连接按钮操作决定 Codex 的目标。

Agent 已持有会话时，新发布的文档不会抢走旧会话。应先让 Agent 提交、取消或释放旧会话，再对新目标发起调用。

## 三、推荐协作顺序

1. 在钢琴卷帘或 Patterns 页整理人工起点，并保存工作副本。
2. 在 Patterns 页选择 Agent 的 Pattern 和初始范围。
3. 在 AI / MCP 页连接当前文档。
4. 向 Agent 描述音乐目标和必须保留的声部。
5. Agent 工作期间播放、查看，但不尝试人工写入。
6. 对范围扩展和 Pattern 切换逐次批准或拒绝；信任 Agent 时可分别开启对应的“总是允许”选项。
7. 在审核区比较 Baseline、Proposal 和 Current document。
8. 应用或拒绝整份提案；开启“总是接受提交”时则核对返回的 `applied` 或 `pending_review` 状态。
9. 播放验证结果，并按需使用 Undo 或开始下一轮协作。

这条流程把人工钢琴卷帘编辑、Agent 私有候选和最终文档修改分开：用户始终保留目标选择和授权决定；除非用户显式开启“总是接受提交”，最终应用仍由人工操作。
