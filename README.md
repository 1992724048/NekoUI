# NekoUI

> **状态：已冻结（legacy）** — 2026-08-13 起停止开发，仓库归档保留，待未来条件成熟再恢复。

## 冻结说明

NekoUI 是一个 Windows C++ GUI 框架实验项目（DirectX 11 渲染），原作为「工具/表单类 Windows 桌面软件」的技术储备。经技术选型评估（Flutter Windows / WebView2 / 继续自研三方向对比），产品方向已转向 **C++ 宿主 + WebView2 + Vue 3** 架构，本仓库冻结为 legacy。

冻结决策要点：

- 自研框架距最小可用还需约 1.2万-2.5万行基础设施（滚动/列表、多行文本、图片、弹窗、控件矩阵、测试等），1 年产品线内风险过高
- 产品目标要求「前台允许高占用、后台驻留必须瘦身」——WebView2 关闭后浏览器进程树确定性释放为官方保证；Flutter 引擎驻留内存无法释放且无官方销毁重建模式
- 新方向最大化复用既有技能（C++ 核心逻辑 + Vue 3/Tailwind），并消除框架级 API 设计负担

## 冻结时状态快照

- 自写代码约 6,200 行 / 96 文件；AGENTS.md 与实际代码高度吻合
- 已实现：D3D11 渲染模块化、Win32 事件全链路、3 线程模型、HCT Material You 色彩引擎（与 material_color_utilities 0.13.0 对齐）、12 种速率曲线动画引擎、5 个基础控件（Button/Center/Column/Row/TextField）
- 可复用资产：HCT 色彩引擎（`Style/ColorScheme.cpp`）、动画引擎（`Component/Animation.hpp`）、HCT 调试工具（`tools/color-picker/`）

## 已知问题

- 矩形语义混用：布局层按 `{x,y,x2,y2}` 存储 bounds，`Drawer` 按 `{x,y,w,h}` 消费——x=0 时巧合正确，Center 布局/偏移后绘制错位
- `stb_truetype.h` 被 .gitignore 排除且未入库——新 clone 无法构建（恢复开发前需先解除 ignore 并入库）
- 字体路径硬编码 `C:\Windows\Fonts\msyh.ttc`
- TextField 多处 MVP 妥协（近似测宽、hover 顶替聚焦）

## 恢复开发指引

若未来恢复开发，按投入产出顺序处理：

1. 矩形语义 bug——引入 `Rect{x,y,w,h}` 类型 + `from_xyxy`/`from_xywh` 构造，消灭裸 `Vec4I` 歧义
2. 解除 `stb_truetype.h` ignore 并入库；字体资源化
3. 真焦点系统（FocusNode + 键盘导航），替换「hover 顶替聚焦」
4. 滚动 + 剪裁系统（滚轮事件已分发但零消费）
5. 文本系统（多行换行、选区、剪贴板、精确光标）
6. 布局约束（margin/padding、min/max 尺寸）
7. 浮层系统（下拉/对话框/tooltip）+ 控件矩阵（CheckBox/List/Table 等）

完整架构、规范与历史变更记录见 AGENTS.md（冻结时保留原样）。
