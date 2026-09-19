# 原型化有界乐段渲染契约

Parent: ../map.md
Type: prototype
Status: open
Blocked by: none

## Question

把现有对话框驱动、以 Order 边界为主的离线导出压缩成一个无 UI、可取消、绑定不可变 Document Revision 的最小原型时，怎样表达 Order occurrence、Pattern 行半开区间、焦点通道与完整混音，并对 Pattern break、jump、loop、delay、插件尾音和终止上限给出可验证且不误导的行为？

## Completion evidence

- 产出一个只用于提高讨论清晰度的原型或可执行探针，并链接资产；不提交生产实现。
- 明确首版支持的 occurrence/range 语义、控制流在范围边界处的行为、完整混音与焦点通道含义、取消和硬终止条件。
- 对暂不支持的 jump、loop、delay、插件尾音或其他情形逐项定义 typed failure，不能静默截断或改写范围。
- 验证渲染前后播放、插件与文档状态得到恢复，重复相同修订和范围可得到同一内容身份。
- 由用户人工检验并接受原型所表达的范围行为后才可解决本票。
