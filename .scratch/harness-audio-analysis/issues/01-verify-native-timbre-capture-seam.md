# 核实原生音色捕获接缝

Parent: ../map.md
Type: research
Status: resolved
Blocked by: none

## Question

钢琴卷帘当前通过 `CModDoc::PlayNote` 进行实时试听；在不复制合成语义、不污染实时音频线程且保留默认 `C-5` 与可选音高的前提下，最小的原生接缝应在哪里、以什么生命周期稳定地产生供人工与模型共同消费的 PCM/WAV、元数据和内容哈希？现有代码中哪些部分可直接复用，哪些能力确实缺失？

## Completion evidence

- 给出带文件与行号的现有调用链、线程/状态所有权和缺失能力证据。
- 比较可行接缝并推荐一个最小接口，明确可选音高、默认 `C-5`、开始/停止、取消、稳定音频身份和原生所有权。
- 给出可自动检查的 PCM/WAV 可重复性、内容哈希、无音乐写入和不进入实时回调的验证办法；未知项必须显式标记。

## Answer

### 结论

最小且可守住线程、状态与身份边界的原生接缝，应放在 **`CModDoc` 所有线程上的文档级异步音色捕获能力**，而不是放在 `CViewPianoRoll`、实时音频回调或 Python Sidecar 中。文档线程只负责校验 `(documentId, revision)`、解析乐器 / Sample、在音频临界区内取得不可变原生音色快照并创建任务；任务随后由自己拥有的、与实时播放 `CSoundFile` 分离的离线渲染器产生 PCM，最后用现有 WAV 编码器封装。Sidecar 和 Harness 只持有任务 / 临时资源句柄，不持有合成器、`CSoundFile` 或文档状态。现有 `CSoundFile` 明确禁止复制，因此“可独立渲染的音色快照 / scratch sound file”是必须新增的能力，而不是可以假定已有的复制构造器（`openmpt-original_ref/soundlib/Sndfile.h:605-615`）。

首版应把“原生音色”严格限定为能完整快照的 OpenMPT Sample、Instrument（含其键盘映射、包络、调音与所引用 Sample）和 OPL patch。第三方插件乐器、外部 MIDI 输出、缺失的外部 Sample 必须返回明确的 `unsupportedTimbreSource` / `missingResource`，不能偷偷回退到抓声卡输出；插件实例复制、插件随机性、外部 MIDI 回录在当前源码中没有可证明的稳定所有权或位精确保证，均属 **未知 / 首版不支持**。

### 现有调用链与所有权

1. 钢琴卷帘的计算机键盘把 `Z...M` 映射为从 `NOTE_MIDDLEC` 开始的十二个半音（`openmpt-original_ref/mptrack/View_pianoroll.cpp:718-724`）；常量本身是 `5 * 12 + NOTE_MIN`，即项目显示语义中的 `C-5`（`openmpt-original_ref/soundlib/modcommand.h:21-28`）。因此新接口的 `pitch` 可选，省略时必须原样取 `NOTE_MIDDLEC`，不能重新发明 MIDI 音高换算。
2. `PreviewPitch` 校验音高与当前选择，按文档是否有 Instrument 决定设置 `PlayNoteParam::Instrument` 或 `Sample`，再调用 `CModDoc::PlayNote` 并保存返回的预听通道（`openmpt-original_ref/mptrack/View_pianoroll.cpp:746-755`）。松键 / 松鼠标后，`StopPreview` 用同一音高、乐器和预听通道调用 `CModDoc::NoteOff`（`openmpt-original_ref/mptrack/View_pianoroll.cpp:758-766`）。`PlayNoteParam` 已经是可复用的音高、Instrument、Sample、音量、声像和通道参数载体（`openmpt-original_ref/mptrack/Moddoc.h:83-106`）。
3. 当前 `CModDoc::PlayNote` **不是离线生成接口**。若该文档尚未播放，它会重置文档 `CSoundFile` 的通道、置暂停标志并让 `CMainFrame::PlayMod(this)` 启动真实播放（`openmpt-original_ref/mptrack/Moddoc.cpp:1062-1078`）；随后在 `CriticalSection` 内选择额外通道并直接改写文档所拥有的 `m_PlayState.Chn`（`openmpt-original_ref/mptrack/Moddoc.cpp:1080-1118`），再执行原生 `InstrumentChange`、OPL `Patch`、`NoteChange` 和插件 `MidiCommand` 语义（`openmpt-original_ref/mptrack/Moddoc.cpp:1095-1128`、`openmpt-original_ref/mptrack/Moddoc.cpp:1157-1177`）。`NoteOff` 同样在音频临界区内操作这些预听通道，并处理 Sample、包络、OPL 和插件释放（`openmpt-original_ref/mptrack/Moddoc.cpp:1211-1267`）。
4. 真正的实时声卡回调最终对当前 `m_pSndFile` 调用 `CSoundFile::Read`（`openmpt-original_ref/mptrack/MainFrm.cpp:905-919`）；音频线程身份和填充锁由 `CMainFrame` 管理（`openmpt-original_ref/mptrack/MainFrm.cpp:821-841`）。因此在该回调里复制 PCM、写文件、算哈希或发 IPC 都会污染实时路径，不能作为模型输入的捕获接缝。
5. 预听路径不写 Tracker Pattern：钢琴卷帘源码明确把它限定为 preview、把 recording 排除在外（`openmpt-original_ref/mptrack/View_pianoroll.cpp:718-719`），用户文档也承诺松键停止且不录入 Pattern（`USER_GUIDE.md:44-48`）。但它会改写实时 `PlayState`，所以“无 Pattern 写入”并不等于可安全复用同一个实时 `CSoundFile` 做确定性渲染。
6. 文档已有生命周期身份与单调修订：构造时生成 `document-N`（`openmpt-original_ref/mptrack/Moddoc.cpp:139-147`），`SetModified(true)` 推进 `m_aiRevision`（`openmpt-original_ref/mptrack/Moddoc.cpp:169-177`），并公开 `AIIdentity()` / `AIRevision()`（`openmpt-original_ref/mptrack/Moddoc.h:167-172`）。捕获请求应在文档线程原子绑定这两个值，并在快照完成前后拒绝修订漂移。

### 可复用部分与确实缺失的能力

可直接复用，但应抽到不依赖 UI / 实时播放器的原生函数：

- `PlayNoteParam` 的参数语义，以及 `CModDoc::PlayNote` / `NoteOff` 中选择映射 Sample、初始化通道、包络 / NNA、OPL 和释放音符的代码；两条消费者（实时试听和离线捕获）必须调用同一原生 `TriggerPreviewNote` / `ReleasePreviewNote` 内核，不能在 Harness 重写合成语义（`openmpt-original_ref/mptrack/Moddoc.cpp:1085-1188`、`openmpt-original_ref/mptrack/Moddoc.cpp:1211-1267`）。
- 现有离线块读取适配器 `ReadInterleaved`，它用 `AudioTargetBuffer` 包装目标后调用 `CSoundFile::Read`（`openmpt-original_ref/mptrack/Mod2wave.cpp:45-52`）；现有整曲导出也是在普通控制流的循环中读取并交给编码器，而不是由声卡回调写文件（`openmpt-original_ref/mptrack/Mod2wave.cpp:1131-1163`、`openmpt-original_ref/mptrack/Mod2wave.cpp:1204-1233`）。
- WAV 流编码器：它写确定的格式头、交错小端样本并显式 finalize（`openmpt-original_ref/src/openmpt/streamencoder/StreamEncoderWAV.cpp:46-65`、`openmpt-original_ref/src/openmpt/streamencoder/StreamEncoderWAV.cpp:71-97`、`openmpt-original_ref/src/openmpt/streamencoder/StreamEncoderWAV.cpp:124-126`）；默认已是 48 kHz、双声道、32-bit float、lossless（`openmpt-original_ref/src/openmpt/streamencoder/StreamEncoderWAV.cpp:136-158`）。关闭 tags 可避免额外非音频身份字段，因为 tags 仅在设置开启时写入（`openmpt-original_ref/src/openmpt/streamencoder/StreamEncoderWAV.cpp:57-63`）。
- 导出取消模式可借鉴：现有进度对话框将取消置为 `m_abort`（`openmpt-original_ref/mptrack/ProgressDialog.h:25-51`），渲染循环在块边界检查并退出（`openmpt-original_ref/mptrack/Mod2wave.cpp:1268-1274`）。SHA-256 已由仓库基础设施提供（`openmpt-original_ref/src/mpt/crypto/hash.hpp:86-90`、`openmpt-original_ref/src/mpt/crypto/hash.hpp:192-195`）。

不能直接复用为本需求接缝：

- `CDoWaveConvert` 是整曲 / Order 导出器。它重置并重配 **当前文档的** `CSoundFile`（`openmpt-original_ref/mptrack/Mod2wave.cpp:995-1012`），暂停当前播放、重置播放位置并标记 renderer（`openmpt-original_ref/mptrack/Mod2wave.cpp:1117-1125`），结束后再恢复整曲位置与音频参数（`openmpt-original_ref/mptrack/Moddoc.cpp:2007-2012`）。它没有“一个音色 + 一个音高 + 固定 note-on / release 帧”的接口，也没有不可变快照，直接调用会干扰人工试听。
- `InitRenderer` / `StopRenderer` 只是给传入 `CSoundFile` 切 `m_bIsRendering` 并挂起 / 恢复插件（`openmpt-original_ref/mptrack/MainFrm.cpp:2947-2961`），不提供隔离、复制、任务身份或内容哈希。
- 当前原生 Pattern facade 只接受八个 Pattern / Sequence 工具，其他名称返回 unsupported（`openmpt-original_ref/mptrack/AIPattern.cpp:305-312`）；Sidecar 的公开工具表也只有这些 Pattern 工具（`sidecar/openmpt_mcp.py:29-51`）。音色捕获任务、临时音频资源、元数据与哈希协议均尚不存在。
- `CSoundFile` 的 PRNG 默认从全局随机源初始化（`openmpt-original_ref/soundlib/Sndfile.cpp:113-122`）；现有整曲导出的 dither 也从应用 PRNG 构造（`openmpt-original_ref/mptrack/Mod2wave.cpp:1007-1018`）。若要宣称位精确，捕获路径必须禁用 dither 或使用固定种子，并把混音 / 重采样版本和设置写进元数据。仓库已有 playback test 用确定性 random device 替换 `CSoundFile` PRNG 的先例（`openmpt-original_ref/soundlib/PlaybackTest.cpp:640-662`）。

### 推荐的最小接口与生命周期

建议原生层暴露一个文档拥有的能力（名称可调整，语义不可缩减）：

```cpp
StartTimbreCapture({
  expectedRevision,
  instrumentOrSample,
  optionalPitch = NOTE_MIDDLEC,
  noteOnFrames,
  releaseFrames,
  format = {48000, 2, float32, ditherNone}
}) -> { jobId }

GetTimbreCapture(jobId) -> queued | rendering | completed{metadata, pcmSha256, wavSha256, resource} | failed{code}
StopTimbreCapture(jobId) -> stopping | finalizing
CancelTimbreCapture(jobId) -> cancelling | cancelled
```

`noteOnFrames` 是自动捕获“开始到停止（note-off）”的确定性边界；渲染器在第 0 帧调用共享的 `TriggerPreviewNote`，到该固定帧调用共享的 `ReleasePreviewNote`，再渲染恰好 `releaseFrames`。`StopTimbreCapture` 供人工长按等交互路径使用：它在下一个声明过的离线渲染块边界执行同一 release，并把实际 `noteOffFrame` 写入元数据；这种交互产物不能冒充固定请求的可重复性 fixture。不要用 UI 松键的墙钟时间定义模型音频，否则同一请求无法复现。现有实时 `PlayNote` / `NoteOff` 仍可用于即时扬声器试听；人工与模型共同消费的是捕获任务完成后同一份临时 WAV。`Cancel` 是任务控制，不等同于音符 release：它只在块边界观察原子取消标记，停止渲染、删除未发布临时文件，并返回 `cancelled`，绝不发布半个 WAV。

生命周期为：`queued -> snapshotting(owner thread) -> rendering(worker-owned isolated renderer) -> stopping -> finalizing -> completed`；固定帧自动 release 可直接从 `rendering` 进入 `finalizing`，任一步可转 `failed`，渲染阶段可经 `cancelling -> cancelled`。文档关闭或其生命周期身份失效时取消未完成任务；已完成临时资源按 Harness 会话寿命回收。只有 `finalizing` 成功后才用原子 rename / 句柄发布 WAV 与 metadata。原生文档 / capability manager 拥有任务注册表和快照，任务拥有 scratch renderer、PCM、临时文件及取消标记；Sidecar 仍保持无状态，只传递 job/resource handle。若未来支持有 GUI 线程亲和性的插件，是否还能使用 worker-owned renderer 是未知项，必须另做能力探测；这不改变首版拒绝插件的结论。

稳定音频身份至少包含：`documentId`、捕获的 `revision`、Instrument / Sample locator、解析后的 native-source fingerprint、实际 `pitch`、`noteOnFrames`、`releaseFrames`、sample rate、channels、sample format、dither mode、混音 / 重采样设置及 renderer schema/build version。另计算两个哈希：`pcmSha256` 覆盖规范化后的交错 PCM 字节；`wavSha256` 覆盖 finalize 后完整 WAV 字节。跨容器的稳定音频 ID 使用 `timbre-pcm-v1:<pcmSha256>`，WAV 哈希只用于验证交付文件没有变化。内容哈希不能由请求参数代替，因为真实 Sample、包络、OPL 与混音器行为才决定声音。

### 自动验证办法

1. **可重复性与哈希**：用纯 Sample Instrument、带键盘映射 / 包络的 Instrument、OPL 三类固定 fixture；同一 `(documentId, revision, source, request, rendererVersion)` 连续捕获两次，断言 PCM 字节、`pcmSha256`、完整 WAV 字节和 `wavSha256` 分别完全相等。解析 WAV 并断言 48 kHz、2 声道、float32、帧数严格为 `noteOnFrames + releaseFrames`。默认不传 pitch 与显式传 `NOTE_MIDDLEC` 必须同哈希；改传另一合法音高应有不同 metadata，且对有音高响应的 fixture 应有不同 PCM 哈希。WAV encoder 默认格式与无条件可用性已有源码依据（`openmpt-original_ref/src/openmpt/streamencoder/StreamEncoderWAV.cpp:136-164`）。
2. **无音乐写入 / 无实时状态污染**：调用前后分别序列化所有 Sequence、Order、Pattern cells、Instrument / Sample 数据并做测试哈希，同时记录 `AIRevision()`、`IsModified()`、Undo 深度、实时 `m_PlayState` 和当前播放位置；完成、失败、取消三条路径都必须逐项相等。现有修订只在 `SetModified(true)` 时推进（`openmpt-original_ref/mptrack/Moddoc.cpp:169-177`），但测试不能只看 revision，因为旧预听路径会改 `PlayState` 而不一定标记文档修改。
3. **不进入实时回调**：给捕获入口、每次离线 `CSoundFile::Read`、WAV write/finalize、SHA-256 和 IPC 各记录线程 ID，并断言它们不等于 `CMainFrame::m_AudioThreadId`；离线读取函数入口增加 `MPT_ASSERT(!CMainFrame::InAudioThread())`。同时在声卡播放与 MCP 流量并发的 native integration test 中复用现有回调探针：回调每 buffer 已调用 `AI::AudioCallbackIpcCheck`（`openmpt-original_ref/mptrack/MainFrm.cpp:905-916`），现有测试会断言 trace 既出现探针又没有 `VIOLATION`（`sidecar/test_native_integration.py:256-261`）。新测试还必须断言捕获专用 trace 从未出现在回调线程，而不只是“IPC 没进入回调”。
4. **开始 / 停止 / 取消**：用很短的固定帧块断言第 0 帧触发、`noteOnFrames` 处 release、总帧数固定；在 snapshotting、rendering、finalizing 各注入取消，断言没有 completed resource、临时文件被清理、任务终态为 cancelled，文档状态不变。文档关闭竞态应返回 `documentGone` / cancelled，不得解引用旧 `CModDoc`；当前服务已在分派时要求 GUI / 文档所有线程并重查文档生命周期，可沿用这一边界（`openmpt-original_ref/mptrack/AIService.cpp:709-727`）。
5. **失败分类**：缺失 Sample、非法音高、revision conflict、资源上限、编码失败、取消、第三方插件和外部 MIDI 分别有稳定错误码；失败不得生成可消费 WAV。固定最大 note-on、release、总 PCM 字节和任务期限，防止无限包络 / loop。

### 显式未知项

- 尚无代码证明第三方插件实例可以安全克隆、脱离实时设备渲染或跨运行位精确；首版必须拒绝，后续只能在逐插件能力声明与 fixture 通过后开放。
- 纯 Sample / OPL 在**同一 build、CPU 路径和固定设置**下应能做字节重复性验收，但跨编译器、跨 CPU 浮点实现或 renderer 版本的位精确性尚未证明；因此 renderer build/schema 必须参与缓存键，不能承诺跨版本哈希稳定。
- Instrument / Sample 的完整、无悬空引用深拷贝 API 当前缺失；实现前须决定是新增专用 `TimbreSnapshot`，还是构造最小 scratch `CSoundFile`。无论选哪种，都必须由共享原生 trigger/release 内核解释，不能让 Python/Harness 复制键盘映射、包络或 OPL 语义。
