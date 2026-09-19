# Ultra review brief: Harness 音频分析内核 Wayfinder 地图

## Review request

Read only this brief. Review the completed chart for destination fidelity, coverage, ticket sharpness, dependency correctness, scope drift and hidden assumptions. Distinguish **blockers** from **suggestions**. Cite the supplied conversation, repository evidence, map section or ticket name for every finding. Do not edit files, explore the repository, or silently replace accepted user decisions.

## Original user input

The user asked to add a Harness core with native audio-input models that can (1) audition and classify a timbre, (2) audition a passage and judge whether it sounds harmonious, while retaining a human audition interface and marking whether each conclusion came from a human or multimodal AI. In the current request, the user explicitly asked to create a **new Wayfinder map based on the existing plan**.

## Accepted conversation decisions

- A clear and common timbre may return a direct instrument name; otherwise use three composable dimensions: instrument family, arrangement role and audible character.
- If a passage has an obvious problem, state it directly; otherwise discuss pitch/harmony, rhythm/coordination, timbre/mix and overall feel.
- Use cloud APIs and choose among Qwen3.5 Omni Plus, Gemini 3.1 Pro Preview and Gemini 3.8 Flash by evidence, not model name.
- Expose timbre and passage audition as MCP tools.
- Use PydanticAI as the first Harness base. Keep provider adapters replaceable and keep domain contracts independent of PydanticAI internals.
- Timbre audition reuses the Piano Roll instrument audition path, accepts an optional pitch and defaults to C-5.
- Passage audition reuses or adds bounded audio-range export.
- Saved projects lazily create a missing project audio-sub-agent `AGENT.md`; the main Agent may edit it and the audio sub-agent only reads it. Unsaved projects do not create a project file but still work with system and main-Agent prompts and tell the user that saving enables project prompts.
- Aside from debug input/output logs, audio, tasks and review results are temporary session data.
- Human and AI opinions are separate, revision-bound, use the same audio, carry trustworthy provenance, and never directly modify music.

## Repository evidence supplied by Pi Explorer

- The accepted plan and its unresolved implementation-preparation details are in `../ai-collaborative-openmpt/issues/36-add-harness-audio-analysis-core.md`.
- The Sidecar is currently standard-library-only and exposes eight Pattern tools; no dependency manifest exists (`sidecar/README.md:29-31`, `sidecar/openmpt_mcp.py:30-49`).
- The app envelope currently accepts only `attach` and `call` (`openmpt-original_ref/mptrack/AIService.cpp:204-206,724`).
- Document Revision is `m_aiRevision`, and the existing app/Sidecar path enforces owning-thread dispatch and typed failures (`openmpt-original_ref/mptrack/Moddoc.h:139,171`; `AIService.cpp:711`; `sidecar/test_native_integration.py:256-262`).
- Piano Roll timbre preview calls `CModDoc::PlayNote` and `NoteOff`; it exposes no stable PCM/file/hash capture (`View_pianoroll.cpp:746-766`; `Moddoc.cpp:1062-1090`).
- Existing passage export is modal and Order-bounded. Its lower renderer writes to an `std::ostream`, mutates playback/render state and does not directly express Pattern occurrence, half-open row ranges or focus channels (`Moddoc.cpp:1689-1714`; `mod2wave.cpp:1074-1125,1241,1279`).
- Existing Sidecar and native integration tests already exercise a real named-pipe peer, all current tools, typed failures and the realtime-callback exclusion invariant (`sidecar/test_sidecar.py:25-26,221-231`; `sidecar/test_native_integration.py:25,358-407`).
- Existing fixtures target Pattern collaboration; validation already renders via `openmpt123`, but there is no dedicated audio-review corpus (`test-fixtures/README.md:1-24,71-73`).
- `CONTEXT.md` defines Audio Preview and Mini Audio Reviewer as revision-bound, non-mutating evidence subordinate to Score Context.

## Destination

Produce an implementation-ready Harness audio-analysis contract: native capture and bounded render seams, a unified audio envelope, PydanticAI/Sidecar deployment boundary, review and task lifecycles, MCP surface, human UI, project prompt rules and a phase-0 verification gate are all decided with observable acceptance evidence. This map plans those decisions; it does not implement the product.

## Map body summary

- Notes preserve all accepted directions, existing domain vocabulary, native ownership of audio semantics, same-hash human/AI review and existing app/Sidecar safety boundaries.
- Decisions so far is empty because this is a new map.
- Fog covers deeper tracker control-flow semantics, optional Score Context/Piano Roll evidence, richer human identity/history, and the eventual default cloud model after real fixtures/adapters exist.
- Out of scope includes implementation, changing Harness framework, local models, persistent review databases, automatic music mutation, mandatory full Context Packages, Python reimplementation of audio semantics, MCP lifecycle redesign and premature default-model selection.

## Created tickets

1. **核实原生音色捕获接缝** — `research`; blockers: none. Locate the minimal native stable-audio seam behind live `PlayNote` audition.
2. **原型化有界乐段渲染契约** — `prototype`; blockers: none. Make exact range/control-flow behavior concrete enough for human acceptance.
3. **核实候选提供商的真实音频能力** — `research`; blockers: none. Verify real API identifiers, audio/structured-output limits, credentials, price and quota.
4. **决定 Harness 的进程与依赖边界** — `grilling`; blockers: none. Reconcile PydanticAI with the standard-library-only supervised Sidecar.
5. **决定统一音频包络与资源上限** — `grilling`; blockers: 核实原生音色捕获接缝, 原型化有界乐段渲染契约, 核实候选提供商的真实音频能力.
6. **定义评审结果与来源契约** — `grilling`; blockers: 核实候选提供商的真实音频能力.
7. **定义试听任务生命周期与临时资源保留** — `grilling`; blockers: 决定 Harness 的进程与依赖边界, 决定统一音频包络与资源上限, 定义评审结果与来源契约.
8. **定义 MCP 试听能力面** — `grilling`; blockers: 定义评审结果与来源契约, 定义试听任务生命周期与临时资源保留.
9. **原型化人工试听与提交工作流** — `prototype`; blockers: 决定统一音频包络与资源上限, 定义评审结果与来源契约, 定义试听任务生命周期与临时资源保留, 定义 MCP 试听能力面.
10. **决定项目级音频 Agent 提示词契约** — `grilling`; blockers: 决定 Harness 的进程与依赖边界.
11. **定义阶段 0 的验证门与实施交接** — `grilling`; blockers: all ten preceding tickets.

## Tracker constraints

- Local Markdown map: `.scratch/harness-audio-analysis/map.md`.
- Child tickets: `.scratch/harness-audio-analysis/issues/NN-<slug>.md`.
- Ticket types are exactly `research`, `prototype`, `grilling`, or `task`.
- `Blocked by: NN, NN` uses blocker-to-blocked direction.
- Frontier is open, unblocked, unclaimed children ordered by number.
- Map is an index; resolved answers live only in tickets and are linked by one-line gists.

## Unresolved uncertainties for review

- Whether the unified audio envelope truly must wait for all three native/provider investigations, or whether some portions can be decided independently.
- Whether the review schema ticket needs provider research as a blocker, since most semantics were already accepted.
- Whether the prompt-file contract depends only on process ownership or also on MCP task creation/lifecycle.
- Whether the final verification-gate ticket is too broad for one 100K-token decision session.
