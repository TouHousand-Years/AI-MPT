# AI-MPT

简体中文 · [English](README.md)

AI-MPT 是基于 [OpenMPT](https://openmpt.org/) 开发的音乐创作项目。原项目的[官方源码镜像](https://github.com/OpenMPT/openmpt)以及本仓库所用源码的版本记录见 [UPSTREAM.md](UPSTREAM.md)。

本项目在 OpenMPT 的 Tracker 工作流上增加了两项功能：

- **钢琴卷帘（Piano Roll）**：以时间和音高视图查看 Pattern 音符，支持选择、试听和播放跟随。开启实验性编辑后，还可插入、移动、调整长度、复制和删除音符，并将修改写回 Tracker Pattern。
- **AI / MCP 协作**：通过本机 MCP Sidecar 让 Agent 读取 Pattern、修改候选内容，以及管理当前 Sequence 的 Pattern 顺序和切换编辑目标。Agent 的修改先保存在候选提案中；默认由用户在 OpenMPT 内审核并应用整份提案，也可显式开启自动接受提交。

钢琴卷帘编辑默认关闭，当前仍属实验性功能。使用条件、MCP 客户端配置和完整操作步骤请阅读[中文用户指南](USER_GUIDE_CHN.md)。
