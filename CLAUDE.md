# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

NekoUI 是一个基于 Win32 + DirectX 11 的声明式响应式 GUI 框架（类似 Flutter/Compose 的自绘 UI 库），当前处于草稿阶段。核心能力：跨平台渲染后端抽象、响应式 Widget 树、Material You HCT 色彩引擎、12 种速率曲线动画引擎、独立渲染线程 + 线程安全消息队列。

- 语言标准：**C++20（Win32）/ C++latest（x64，Intel 编译器）**
- 命名空间：`neko`（子命名空间 `type`/`engine`/`widget`/`backend`/`platform`/`device`/`component`/`style`）
- 所有源码在 `NekoUI/NekoUI/` 下，入口为 `NekoUI/main.cpp`（含 demo UI）
- 项目已有完整 AGENTS.md（本仓库的 AI 入职文档，含行级架构细节）——但它当前在**工作区被删除**（`git status` 显示 `D AGENTS.md`）。如需恢复：`git checkout -- AGENTS.md`。CLAUDE.md 是精简版。

## Build

- 构建：在 VS 2022+ 打开 `NekoUI.slnx`，或命令行 `msbuild NekoUI.slnx`
- 配置矩阵（`.slnx` 平台名 `x64`/`x86`，对应 vcxproj 内部配置 x64/Win32）：

| 配置 | 工具链 | 语言标准 |
| --- | --- | --- |
| Debug/Release × Win32 | MSVC v145 | C++20 |
| Debug/Release × x64 | **Intel C++ 2026** | C++latest |

- **x64 构建需要本机安装 Intel oneAPI 2026 工具集**（TBB/IPP/MKL 等），否则无法编译
- 输出目录 `$(SolutionDir)$(Platform)-$(Configuration)/`，中间目录 `.tmep/`（拼写即如此）
- Debug 配置需显式声明 `LocalDebuggerCommand` + `DebuggerFlavor=WindowsLocalDebugger`，否则 F5 无法启动调试
- 无测试项目、无包管理器（无 vcpkg/NuGet）；不运行 `clang-tidy` 静查时验证依赖用户自行编译（见「验证」）

## Architecture

分层（自底向上）：`platform` → `backend` → `device`/`component` → `engine` → `widget` → `style`。

### 事件流与渲染管线

```
WM_* → Platform::translate_event → MsgPump::push_msg（SPSC 环形缓冲）
  → EventRouter::dispatch（std::visit 分发）
    → HitTester::hit_test → Widget::input / RenderScheduler::set_pending_size / 主题更新
```

```
Engine::render_frame：consume_resize → 帧首合并重建(tree_dirty_) → root->layout({0,0,w,h})
  → backend->begin → draw_widget 前序 DFS 递归绘制 → backend->end → 清除脏标记
```

### 线程模型

| 线程 | 职责 |
| --- | --- |
| 主线程 | Win32 消息循环（`GetMessageW`/`DispatchMessageW`） |
| 渲染线程 | `RenderScheduler`（`std::jthread`），动画期间 16ms 节拍出帧 |
| 消息线程 | `MsgPump`（`std::jthread`），有界 SPSC 环 + `counting_semaphore<32>` |

树结构并发用 `TreeManager::mutex_`（`std::shared_mutex`）保护，三类持有者见「关键不变量」。

### 关键不变量（务必遵守，避免踩坑）

1. **`build<T>()` 死锁禁令**：Widget 自定义 `build(Context&)` 在 `register_widget` 的锁内执行，**禁止在内部调用 `build<T>()`**（同线程重锁自死锁）。`build<T>()` 插入 `children_` 前取 `unique_lock(tree_mutex)`；`render_frame` 在 layout+draw 外围持 `shared_lock`；`HitTester` 遍历同持 `shared_lock` 读锁。
2. **重绘触发**：输入事件本身**不**触发重绘；只有 Resize/DpiChange/ThemeChanged 或控件内部 `mark_dirty()` 才调 `request_frame()`。新增交互逻辑时记得 `mark_dirty()`，否则画面不动。
3. **hover 离开清除**：`EventRouter::last_mouse_target_` 在 MouseMove 命中变化时向旧目标补派同一事件（旧按钮 `is_inside=false` → hover 还原），再派发新目标。修改事件路由时不要破坏这条补派链。
4. **Widget 所有权**：`children_` 用 `shared_ptr` 强持有（`MutableWidget` 变体 `monostate/list/vector/shared_ptr`），树持有所有权。曾因 `weak_ptr` 无强持有者导致子控件创建即销毁。
5. **`Backend/stb_truetype.h` 已被 gitignore，不跟踪**——删除后无法从 git 恢复（属嵌入式第三方库，重下后放回原位）。

### 关键类速览

| 类 | 职责 |
| --- | --- |
| `Engine` | 总控制器：拥有全部子系统并 `std::bind` 连接回调；`set_root_widget<T>()` 工厂；`render_frame()`/`rebuild()`；私有静态 `draw_widget()` 集中式 DFS 绘制 |
| `Context` | 引擎共享上下文：`mark_dirty`/`widget_dirty`/`anim_inc`/`anim_dec` 回调、Mouse/Keyboard 弱引用、`tree_mutex`、root 弱引用、`ColorScheme` |
| `TreeManager` | 树数据：root/focus 原子指针、ID→Widget、index→Widget 映射、焦点导航、`shared_mutex` |
| `WidgetBuilder` | 构建遍历：递归注册到 TreeManager 映射并调用各 Widget `build()` |
| `HitTester` | 命中测试：递归反向遍历（z 序），返回顶层命中 Widget |
| `WidgetVisitor` | 工具模板：`visit_children()` 统一分发 `MutableWidget` 四种变体 |
| `EventRouter` | `std::visit` 事件分发（含 `last_mouse_target_` hover 补派） |
| `InvalidationTracker` | 脏标记：atomic dirty + animation 计数 + 脏 Widget 列表（`shared_mutex`） |
| `RenderScheduler` | 渲染线程：`request_frame()`/`set_pending_size()`/`stop()`（join 线程，self-id 守卫） |
| `MsgPump` | 线程安全消息队列（SPSC 32 槽 + 信号量 + jthread） |
| `Backend` | 绘制抽象基类（策略模式）：`draw_rect_fill`/`draw_rect`/`draw_line`/`draw_circle_fill`/`draw_text`、`begin`/`end` |
| `Platform` | 平台基类（单例 + 工厂注册宏 `NEKO_REGISTER_PLATFORM`）：事件翻译、IME(TSF)、11 个窗口操作 |
| `ColorScheme` | Material You 36 色调色板；完整 HCT 引擎（sRGB↔XYZ↔CAM16↔HCT，牛顿迭代求解），`light(seed)`/`dark(seed)` 工厂 |
| `Widget` | 基类：`layout`/`draw`/`build`/`event`/`input`/`hit_test` 虚方法；Builder API `build<T>()`/`children(fn)`/`parent()` |
| `ValueState<T>` | 响应式值，赋值触发 `mark_dirty()` 回调 |
| `Animation<T, Easing, Time>` | 12 种速率曲线 × in/out/in_out；`AnimationBase` 管理 `is_active()`/`bind(on_start, on_end)` |

### Widget 与样式

- Widget 通过**多重继承 style mixin**（`BackgroundStyle`/`SizeStyle`/`BorderStyle`/`TextStyle`）获得样式属性，**零运行时开销**、无字符串查找、无 hashmap（曾用 `StyleSheet`+`Stylable` CRTP，已移除）
- 布局下放到各 Widget 的 `layout()` 虚方法（草稿期每帧全量布局），`render_frame` 每帧调 `root->layout` 驱动
- 现有 Widget：`Button`（hover 缩放动画 200ms + on_click）、`Column`/`Row`（栈式布局）、`Center`（居中）
- 主题：读注册表 `AppsUseLightTheme`（亮暗）与 `DWM\AccentColor`（强调色，ABGR→RGBA），监听 `WM_SETTINGCHANGE`（`ImmersiveColorSet`）

### 已实现 vs 未实现

- ✅ 渲染后端抽象 + D3D11、HCT 色彩引擎、平台抽象 + 主题检测、响应式 Widget 树 + 焦点导航、Builder API、动画引擎、事件传递、Button hover/缩放动画、关窗线程安全、树加锁
- ❌ 无测试、无裁剪/溢出、无 margin/padding、无最小/最大尺寸约束、无 Wrap/基线对齐

## Conventions

- 格式化：`.clang-format`（LLVM 自定义）——4 空格缩进、220 列宽、花括号不换行、指针左对齐、访问修饰符缩进 -4、命名空间整体缩进、`template<` 无空格、include `<...>` 优先
- 命名：类型/枚举器 PascalCase，函数/方法/变量/参数 snake_case，私有成员尾随 `_`，宏 UPPER_SNAKE_CASE
- 代码：`#pragma once` 统一头文件防护；**尾置返回类型** `auto f() -> T`；优先 `std::shared_ptr`/`unique_ptr`、`std::optional`/`span`/`jthread`/`shared_mutex`/`atomic`、`std::array`、`std::numbers` 常量、`[[nodiscard]]`、设计初始化器 `.x = value`；不可拷贝类显式 `= delete`
- 异常处理：用异常（Async 模型），非流程控制
- 注释：命名优先于注释，只写「为什么」的意图注释；禁止装饰性分隔符/日志式/修复类注释
- 目录按功能域组织（`Backend/`/`Engine/`/`Widget/`/`Component/`/`Style/`/`Platform/`/`Device/`），include 用相对路径

## Verification

- **C++ 编译例外**：纯 VS 项目禁止自行编译（SDK/工具链/Intel 环境依赖不可靠），改完代码后请用户自行编译验证
- 静态检查（可选）：项目无 `compile_commands.json`；可在 `.temp/` 下写最小 `compile_commands.json`（`clang-cl /std:c++latest /EHa /I <项目根>`）后跑 `clangd --check` / `clang-cl -fsyntax-only` / `clang-tidy`（`modernize-*`/`performance-*`/`readability-*`/`bugprone-*`）作交付证据
- 无自动化测试：改动后用 demo（`main.cpp` 示例 UI）人工验证交互与渲染

## Dependencies

| 依赖 | 用途 | 来源 |
| --- | --- | --- |
| DirectX 11 / DXGI / DirectXMath | 渲染 | Windows SDK |
| Intel oneAPI 2026（仅 x64） | TBB/IPP/MKL/DAL/MPI | Intel 工具集 |
| stb_truetype.h | 字体光栅化（内嵌，gitignored） | 第三方单头文件 |
| MSVC v143/v145 | C++ 标准库 | VS 2022 |
