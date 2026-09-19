# 决定 Harness 的进程与依赖边界

Parent: ../map.md
Type: grilling
Status: open
Blocked by: none

## Question

在现有 MCP Sidecar 是标准库、客户端监督、无项目状态的 Python 进程，而已接受方案要求 PydanticAI 的条件下，PydanticAI 应嵌入现有进程、作为受监督伴随进程，还是位于另一条明确边界；依赖安装、启动、崩溃隔离、stdout 纯净、凭据所有权、取消和版本兼容分别由谁负责？

## Completion evidence

- 比较至少“嵌入 Sidecar”和“受监督伴随进程”两种边界，记录依赖、启动、升级、故障隔离与调试成本。
- 选定一个边界并给出组件/生命周期图，逐项指派 stdout、凭据、网络、取消、超时、版本和临时文件所有权。
- 说明 PydanticAI 内部类型如何被限制在适配层，并给出进程崩溃、能力不匹配和缺少依赖时的可观察失败信号与回滚点。
