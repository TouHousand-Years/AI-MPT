# 原型化人工试听与提交工作流

Parent: ../map.md
Type: prototype
Status: open
Blocked by: 05, 06, 07, 08

## Question

在现有 AI 页面/下部 Review 视图中，人工试听任务应放在哪里、如何展示范围与 stale 状态、怎样播放与 AI 完全相同哈希的音频、怎样表示本机提交者身份并防止 MCP 冒充；最小交互原型能否让用户清楚地区分人工意见、AI 意见、失败与过期结果？

## Completion evidence

- 产出可操作或高保真最小原型并链接资产，展示请求、加载、播放、提交、AI/人工并列结果、失败、取消和 stale 状态。
- 原型显示范围、Document Revision 与音频身份，并证明人工播放器读取与 AI 请求相同的 `audioContentHash`。
- 选定本机提交者的最小身份表示和受信提交入口；任何 MCP 参数都不能直接声明 `source: human`。
- 按仓库约定暂停，由用户人工检验 UI 行为和来源可辨识性；获得明确接受后才可解决本票。
