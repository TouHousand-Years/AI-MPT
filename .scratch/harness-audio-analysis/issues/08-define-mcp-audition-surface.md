# 定义 MCP 试听能力面

Parent: ../map.md
Type: grilling
Status: open
Blocked by: 06, 07

## Question

`audition_timbre`、`audition_passage` 以及查询状态、读取结果、打开人工任务、提交人工意见和取消任务应如何组成最小 MCP 工具面；每个请求/响应怎样复用 attach、Application Capability、owning-thread、Document Revision 和 typed-failure 边界，同时杜绝伪造人工来源、句柄越权和任何隐式音乐写入？

## Completion evidence

- 列出最终最小工具名和版本化输入/输出 schema，并把每个字段映射到统一音频、评审和任务契约。
- 逐工具记录 owning-thread、attach、Document Revision、授权、取消与句柄访问规则，以及明确的 typed-failure 映射。
- 提供负面用例：伪造 `human`、跨文档/实例句柄、过期句柄、能力不匹配、未授权云请求和任何音乐写入均被拒绝。
- 与项目提示词票核对任务创建时的提示词可见效果，但 MCP schema 不暴露文件系统路径或提供商专属分支。
