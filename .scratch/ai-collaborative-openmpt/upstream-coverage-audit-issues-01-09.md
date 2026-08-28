# Issues 01～09 的原版 OpenMPT 覆盖审计

调查日期：2026-08-28  
审计对象：`.scratch/ai-collaborative-openmpt/issues/01-*.md` ～ `09-*.md`  
上游比较基准：OpenMPT SVN `r25644` / 官方 Git 镜像提交 [`0eafb124cfd15f94e32302734607e71f2c36c84f`](https://github.com/OpenMPT/openmpt/commit/0eafb124cfd15f94e32302734607e71f2c36c84f)

## 结论

按“**完整 issue 的核心问题已经由原版 OpenMPT 解决**，并且**本项目未来的常规更改不需要再改变这项结论**”这一严格双重标准，**01～09 中没有任何一个完整 issue 可以判定为上游重复问题，数量为 0**。

最接近该标准的只是三组**上游已存在的子能力**：

1. issue 02 所盘点的基础 Tracker 数据模型、格式读写、播放/渲染和三类局部 undo；
2. issue 04 可复用的 `ModCommand` / `CPattern` 语义来源，以及 Pattern Clipboard 文本投影；
3. issue 09 可复用的 OpenMPT 安装包、portable ZIP、Portable Mode 和现有打包脚本。

这些子能力都不等于对应 issue 已解决：02 仍缺统一事务边界，04 仍缺 revision-bound AI schema，09 的打包仍会被独立产品身份和 Sidecar Release Unit 直接改变。因此，**不建议以“原版已有”为由删除或合并 01～09 中任何一个 issue**。

## 判定口径与证据强度

- **已证实**：官方手册、官方 issue 状态，或固定提交中的正向源码证据直接支持结论。
- **高概率**：对固定上游源码树进行仓库级关键词、类型和调用路径检查后，没有找到相应产品能力；负面搜索不能构成形式证明，因此保留不确定性。
- **证据不足**：上游虽然有相似组件，但官方资料没有证明它满足 issue 要求的原子性、生命周期或发行保证。

“未来的更改没有影响”不能被理解为对未来提交的绝对预测。本审计只把下列结论视为较稳定：固定 `r25644` 的历史事实不会变化；成熟、正向存在的上游基础能力可作为下游起点；而依赖独立产品身份、AI 协议、Sidecar、事务层或新 UI 的结论会被本项目实现直接影响，不能称为稳定地“已由上游解决”。

上游新鲜度也已核验。调查时官方 `master` 为 [`55895cb0898d03b676d6d1771f567f7040dd2605`](https://github.com/OpenMPT/openmpt/commit/55895cb0898d03b676d6d1771f567f7040dd2605)，它相对 `r25644` 只有一个提交、一个文件变化，内容是 Sample 页 Mix Paste / Insert Paste 保留原 Sample 路径，与 01～09 的架构和产品能力无关（[官方 compare](https://github.com/OpenMPT/openmpt/compare/0eafb124cfd15f94e32302734607e71f2c36c84f...55895cb0898d03b676d6d1771f567f7040dd2605)）。因此，固定基准与调查日的最新原版之间没有一个可能补齐 03～08 的隐藏功能窗口。

## 逐项审计

| Issue | 核心技术症状 / 问题 | 原版 OpenMPT 的实际覆盖 | 是否满足严格双重标准 | 证据强度 |
| --- | --- | --- | --- | --- |
| 01 · Pin upstream baseline | 需要为独立派生项目确定不可漂移的 SVN/Git 基准、导入历史、归属和选择性同步规则。 | 上游明确说明开发发生在 SVN，GitHub 只是可能被 rebase 的只读镜像；这解决了“上游事实从哪里取”，却不会替派生项目选择基准、创建永久标签或制定同步策略。 | **否**。这是下游治理决策，不是原版产品缺陷；其稳定性来自本项目主动 pin，而不是上游已实现该工作流。 | 已证实 |
| 02 · Inventory capabilities | `CModDoc` / `CSoundFile` 暴露可变模型，编辑流程散落在 MFC view/controller；undo 分为 Pattern、Sample、Instrument 三套，缺少统一 revision、预校验、跨域原子事务、稳定错误和事件。 | 上游确实已经实现丰富编辑、播放、格式读写和三类 undo；但 `CModDoc::GetSoundFile()` 仍返回可变引用，三套 undo 仍是独立类型，不能证明存在 issue 要求的统一 Application Capability 层。 | **否**。上游解决了领域功能，未解决本 issue 识别出的共享安全边界。 | 已证实（已有结构）；高概率（不存在另一套完整 facade） |
| 03 · Piano-roll semantics | 需要一个和 Tracker 同源、同步选择/播放/undo、能表达 note-off/cut/fade、note delay 和 tracker-only effects 的人类 Piano Roll。 | 官方手册仍把“只能使用文本编辑；没有 piano-roll 或乐谱编辑”列为限制；官方功能请求 0000727 仍为 `new` / `open`。源码中存在 Pattern editor 和 Instrument Note Map，但不是 Pattern Piano Roll。 | **否，而且是最明确的反例**。原版官方明确尚未解决。 | 已证实 |
| 04 · Symbolic Score Context | 需要 revision-bound、稀疏、可保留 Tracker 特殊事件与解释闭包的规范 AI 表示；MIDI/MusicXML 只能是有损视图。 | 原版已经定义 `ModCommand`、`CPattern` 和 Pattern Clipboard 序列化，可作为语义来源/紧凑投影；但没有项目 revision envelope、AI Score Context schema、结构化范围身份或 MCP resource。 | **否**。只能判定底层词汇和 clipboard 投影已解决，核心 AI 合同未解决。 | 已证实（底层类型/clipboard）；高概率（AI 合同不存在） |
| 05 · Shared capability boundary | 需要位于 `CModDoc` / `CSoundFile` 之上的协议中立 facade，具有 owning-thread、不可变查询、`expectedRevision`、全量预校验、一次 undo、失败不变、typed errors 和事件。 | 官方固定源码展示的仍是 MFC handler、直接可变 `CSoundFile`、手动 `SetModified()` / `UpdateAllViews()` 与局部 undo。上游没有公开编辑 API 能满足这一完整合同。 | **否**。这正是必须新增的深层架构层；未来 Piano Roll、AI 编辑和事务实现都会影响它。 | 高概率 |
| 06 · Sidecar IPC lifecycle | 需要 client-supervised MCP Sidecar、同用户 named pipe、实例发现/握手、revision/event 重同步、写租约、取消、receipt、配额与崩溃清理。 | 上游只有用于“另一 OpenMPT 实例打开文件”的 `IPCWindow`，以及面向 VST 插件进程隔离的 `PluginBridge`；二者都是实现参考，不是 named-pipe MCP broker，也没有 revision、lease、receipt 或资源合同。 | **否**。这是全新的集成和安全边界，且未来协议与打包更改会直接影响它。 | 已证实（既有 IPC 的用途）；高概率（MCP 能力不存在） |
| 07 · AI context contract | 需要把 Score Context、离线 Audio Preview、AI Piano Roll 和多模态 Reviewer opinion 绑定成同 revision/range 的全有或全无 Context Package，并定义缓存、隐私、staleness 和 job 生命周期。 | 原版能播放/离线导出音频，但没有 Score Context、AI Piano Roll、Reviewer、Context Package 或相关 revision/resource 生命周期。 | **否**。上游的音频渲染只是一个可复用组件，不能覆盖该合同。 | 高概率 |
| 08 · Reviewable change workflow | 需要 immutable Change Proposal、Review Units、partial acceptance、accepted-subset hash、匹配 audition、高影响确认、stale-base 拒绝、单次原子 apply 与 Operation Receipt。 | 原版有编辑命令和局部 undo，但没有 proposal/candidate state、accepted subset、revision conflict、三方对比或跨域单事务。 | **否**。传统 undo 不能推出 proposal-first 审核工作流，未来 UI 和事务层实现会直接改变它。 | 高概率 |
| 09 · Distribution model | 需要独立身份、BSD 归属、App + Sidecar + helpers 的版本锁定 Release Unit、**无需管理员权限的 per-user installer**、portable ZIP、Local App Data/runtime 分层、手动更新策略、兼容性承诺和选择性上游 intake。 | 原版已经发布统一 installer 和各架构 portable ZIP，也有 Portable Mode 与打包/更新脚本；但当前 installer 明确 `PrivilegesRequired=admin`，非 portable 配置默认走 `%APPDATA%\OpenMPT`，程序还能定期自动检查并可选择自动安装更新。这些都与 issue 09 的下游约束不等价。 | **否**。通用打包基础已存在，但现有策略本身就要修改，且未来的独立身份、Sidecar、schema 和迁移策略都会继续影响它，未通过第二条件。 | 已证实（包形式及策略差异）；证据不足（原子升级等保证） |

## 逐项来源与推理

### 01：上游来源身份已明确，但派生项目工作流未由上游替你决定

OpenMPT 官方 README 把 GitHub 仓库描述为官方 SVN 的只读镜像，并警告最近历史可能因 SVN revision property 修正而 rebase；官方 contributing guide 同样指向 SVN 开发流程（[固定提交 README](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/README.md)，[固定提交 contributing guide](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/doc/contributing.md)）。`r25644` 与 Git SHA 的对应则由固定提交自身的 `git-svn-id` 正向证明。

这意味着 issue 01 的**事实前提**已由上游给出，且固定 revision 的身份不会被未来上游功能更改影响；但“选择哪个 revision”“如何导入派生仓库”“何时 cherry-pick”属于本项目治理，不能说原版已经解决了完整 issue。

### 02 与 05：上游功能丰富，但共享事务边界确实不存在

固定源码中的 [`CModDoc`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.h#L118-L312) 同时持有 `CSoundFile`、modified state、view update 和 undo managers，并公开返回可变 `CSoundFile &`。[`Undo.h`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Undo.h#L18-L179) 明确是 `CPatternUndo`、`CSampleUndo`、`CInstrumentUndo` 三套专用历史，而不是文档级事务日志。[Pattern 编辑实现](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/View_pat.cpp#L427-L694) 则展示 view/controller 准备 undo、直接修改 Pattern，再自行标记和通知的典型路径。

因此可稳定复用的是模型、领域算法和局部 undo 机制；需要新增的是把这些机制收束为 revision-bound、可预校验、可回滚、跨域原子的协议中立能力。issue 02 的“功能盘点”不是待重做的功能清单，但其架构缺口不能被标记为上游已解决；issue 05 更不能关闭。

### 03：官方仍明确没有 Piano Roll

官方手册的 About 页面把“只能使用文本编辑系统；没有 piano-roll 或 musical score editing”列为当前限制（[OpenMPT 官方 Wiki](https://wiki.openmpt.org/Manual%3A_About_OpenMPT)）。更直接的是官方 issue tracker 的 [0000727: Request: Piano Roll Editor](https://bugs.openmpt.org/view.php?id=727)，调查时仍显示 `Status: new`、`Resolution: open`；维护者讨论也明确指出 legacy module 的 loops、jumps、effects 等使准确 Piano Roll 非平凡。

这不仅证明 issue 03 未由原版解决，还支持 issue 03 所选择的“Tracker 保持权威、Piano Roll 只做同一 Pattern 数据的投影”不是对一个现成功能的重复设计。

### 04：语义素材已有，revision-bound AI 表示没有

原版的 [`ModCommand`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/soundlib/modcommand.h) 和 [`CPattern`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/soundlib/pattern.h) 已经正向定义 Pattern cell、特殊音符、volume/effect command、row/channel 以及 Pattern signature/swing 等语义。原版 [`PatternClipboard.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/PatternClipboard.cpp) 也已经序列化 Tracker Pattern 文本，并支持多 Pattern / Order 相关信息。

所以 issue 04 不应重新发明 Tracker 命令词汇或 clipboard 格式。但源码中没有 issue 要求的 `project_revision`、不可漂移范围、schema version、completeness/omission 标记或 AI resource envelope；把已有文本格式称为完整 Score Context 会丢失 concurrency 和 provenance 约束。

### 06～08：没有上游同类产品能力，只有可复用的低层组件

对固定 `r25644` 源码树以及调查日最新官方树进行项目级检查，在排除 vendored `include/`、`contrib/` 后，`piano.?roll`、`Model Context Protocol`、`score context`、`expectedRevision`、`operation receipt`、`context package`、`review unit`、`agent edit session` 和 `change proposal` 都是 0 命中。官方仓库的公开顶层组成仍是 tracker、sound library、plugin bridge、installer 等传统组件（[固定提交源码树](https://github.com/OpenMPT/openmpt/tree/0eafb124cfd15f94e32302734607e71f2c36c84f)，[官方仓库 README](https://github.com/OpenMPT/openmpt/blob/master/README.md)）。

原版不是完全没有 IPC。[`IPCWindow.h`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/IPCWindow.h) 的 Purpose 明确是“隐藏窗口接收另一个 OpenMPT 实例的文件打开命令”；其枚举只包含打开文件、前置窗口、读取版本/架构/路径、播放当前文档等有限功能，[实现](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/IPCWindow.cpp#L23-L155) 使用 hidden window + `WM_COPYDATA`。`PluginBridge` 则用 communication window、shared memory、signals/events 在 host 与 VST bridge 之间通信（[`BridgeCommon.h`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/pluginBridge/BridgeCommon.h#L582-L657)）。它们可以提供 Windows IPC 和进程生命周期经验，但没有 MCP stdio 翻译、文档能力分派、named-pipe ACL/握手、revision/event、write lease、Operation Receipt 或 Context resource；把它们当 issue 06 已解决会混淆完全不同的协议与信任边界。

这里保留“高概率”而不是“形式证明”，因为对不存在能力的源码搜索总有漏检命名的可能。不过，03 的官方缺口、02/05 的调用结构、以及基准到最新只出现一个无关 Sample 修复，三组独立证据互相吻合。原版已有的离线 WAV 导出入口（[`CModDoc::OnFileWaveConvert`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Moddoc.cpp#L1678-L2027)）和局部 undo 可以成为 07/08 的实现材料，但它们没有 Context Package / proposal 的身份、原子性、审核和生命周期语义。

### 09：包形式已有，派生产品发行模型没有

OpenMPT 官方下载页正向提供 unified installer、各架构 portable ZIP 和 SHA-256（[官方下载页](https://openmpt.org/download)）；官方手册说明 installer 与 portable ZIP 的设置存放差异，并记录 `/portable` 方式（[System Setup](https://wiki.openmpt.org/Manual%3A_System_Setup)）。固定源码也包含 portable package 说明和打包/更新生成器（[`packageTemplate/readme.txt`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/packageTemplate/readme.txt)，[`installer/`](https://github.com/OpenMPT/openmpt/tree/0eafb124cfd15f94e32302734607e71f2c36c84f/installer)）。这些是高价值的实现起点。

但是关键策略和 issue 09 明确相反：固定源码的 Inno Setup 脚本设置了 [`PrivilegesRequired=admin`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/installer/install-multi-arch.iss#L25-L49)，而 issue 09 要求无需管理员权限的 per-user installer；非 portable 模式默认把配置放到 `%APPDATA%\OpenMPT`，找不到时才退到 My Documents（[`Mptrack.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Mptrack.cpp#L1012-L1054)），而 issue 09 选择的是新的独立产品 Local App Data 分层；OpenMPT 还会在合适时机调用 `DoAutoUpdateCheck()`，并有 `InstallAutomatically` 选项（[`Mptrack.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/Mptrack.cpp#L1667-L1677)，[`UpdateCheck.h`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/UpdateCheck.h#L50-L70)，[`UpdateCheck.cpp`](https://github.com/OpenMPT/openmpt/blob/0eafb124cfd15f94e32302734607e71f2c36c84f/mptrack/UpdateCheck.cpp#L1499-L1541)），而 issue 09 的 Developer Preview 只允许显式 Check for Updates 后手动下载。

但 issue 09 的主语是一个**独立命名的派生产品**，且 Release Unit 要新纳入 MCP Sidecar、协议 schema、runtime registration、独立配置命名空间和不同更新源。所有这些都会直接改变 installer、portable layout、升级检测、卸载保留和 license inventory。因此“原版有安装包”不能推出“issue 09 已解决且未来更改无影响”。官方材料也不足以证明 issue 09 所要求的失败回滚、整单元原子升级和跨组件版本锁定保证；这些仍需本项目自行验证。

## 可安全继承、但不能据此关闭 issue 的项目

| 来自 issue | 可以继承的原版能力 | 稳定使用方式 | 仍需本项目完成 |
| --- | --- | --- | --- |
| 02 | `CSoundFile` / Pattern / Order / Sample / Instrument / Plugin 模型；格式 loaders/writers；播放与离线导出；三类 undo | 以固定 `r25644` 为实现基线并做回归测试，不复制重写成熟算法 | 统一 immutable query、revision、transaction、typed error、event 和跨域 undo |
| 03 / 04 | `ModCommand` 特殊音符及 effect 词汇；`CPattern` row/channel 结构；Pattern Clipboard | Piano Roll 和 Score Context 都投影同一上游 Pattern 数据，不创建并行 MIDI 模型 | 同步 Piano Roll、显式有损标记、revision/range/provenance envelope |
| 07 | OpenMPT 的播放/音频导出能力 | 从不可变 snapshot 抽取无对话框、受限、可取消的离线 render 内核 | Context Package、资源句柄、cache/staleness、Reviewer 和隐私授权 |
| 08 | Pattern/Sample/Instrument 局部 undo | 在新的事务层内部复用已有 snapshot/undo 算法 | private candidate、Review Units、partial accept、一次跨域 apply、receipt |
| 09 | installer/portable 源码、Portable Mode、现有 package layout 与 license notices | 作为打包 spike 的参考和初始素材 | 全面重命名、Sidecar Release Unit、per-user 策略、独立数据/更新边界、失败回滚验证 |

## 最终建议

1. **完整 issue 层面：不标记任何 01～09 为“上游已解决 / 无需关注”。**
2. **实现层面：把 02、04、09 中的上游子能力登记为 `reuse upstream`，而不是新实现项。**
3. **回归层面：即使是可继承能力，也不能写成“未来绝不会受影响”；固定 baseline 只隔离未选择的上游提交，不能隔离本项目自己的重构。** Pattern/Piano Roll 同步、统一事务 facade、snapshot render 和重新打包都有可能破坏原版行为，仍需 issue 16 所述兼容性测试。
4. **若筛选条件必须返回 issue 编号，答案应是空集 `[]`；若允许返回子问题，则返回 `02: 基础领域能力`、`04: ModCommand/PatternClipboard`、`09: 原版 installer/portable 基础`，并明确它们都不足以关闭对应 issue。**
