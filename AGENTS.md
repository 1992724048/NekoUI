# NekoUI

## Overview

NekoUI 是一个 Windows C++ GUI 框架（UI 库），使用 DirectX 11 渲染。处于开发早期（草稿状态）。核心架构包括跨平台渲染后端抽象、响应式 Widget 树、Material You HCT 色彩引擎、速率曲线动画引擎、独立渲染线程和线程安全消息队列。

- **语言标准**: C++20 (Win32) / C++latest (x64)
- **渲染**: DirectX 11
- **平台**: Windows (Win32 API)
- **命名空间**: `neko`，子命名空间包括 `neko::type`、`neko::engine`、`neko::engine::internal`、`neko::widget`、`neko::backend`、`neko::platform`、`neko::device`、`neko::component`、`neko::component::ease`、`neko::style`

## Architecture

分层架构，从底层到上层依次为：

### 1. 平台抽象层 (`neko::platform`)

平台事件封装与转换。`Platform` 基类（单例）负责将原生消息（如 Win32 `WM_*`）转换为统一 `Event` 变体。

**支持的事件类型**：
- 鼠标/键盘输入（`MouseMoveEvent`、`MouseButtonEvent`、`MouseWheelEvent`、`KeyEvent`、`CharEvent`）
- 窗口尺寸变化（`ResizeEvent`）
- DPI 变化（`DpiChangeEvent`）
- 窗口销毁（`DestroyEvent`，`WM_DESTROY`）
- 系统主题切换（`ThemeChangedEvent`，`WM_SETTINGCHANGE`）

**主题检测细节**（`Win32.cpp`）：
- Light/Dark 模式：读取注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme`
- 强调色：读取注册表 `HKCU\Software\Microsoft\Windows\DWM\AccentColor`（ABGR 格式 → 转为 `neko::type::Color` RGBA 格式）
- 主题变更监听：`WM_SETTINGCHANGE` 消息，`lparam == "ImmersiveColorSet"` 时刷新缓存

**IME 支持**：`Platform` 提供 IME 输入法控制（TSF 接口，`ITfThreadMgr`、`ITfDocumentMgr`）。

**窗口操作**：11 个方法（show/hide/close/maximize/minimize/restore/destroy/move/resize/focus/set_opacity）。

使用 `register_platform<T>()` 工厂方法 + `NEKO_REGISTER_PLATFORM(T)` 宏注册具体实现。

### 2. 渲染后端 (`neko::backend`)

`Backend` 抽象基类定义绘制接口：`draw_rect_fill`、`draw_rect`、`draw_line`、`draw_circle_fill`、`draw_text`，以及 `resize`、`set_dpi`、`begin`/`end` 帧管理。

`DirectX11` 为其具体实现，包含：
- D3D11 设备/上下文/交换链创建
- HLSL 顶点/像素着色器编译（矩形着色器 + 文本着色器）
- stb_truetype 字体图集生成（含 CJK 字形支持）
- 常量缓冲区管理（`TextCB` 结构体）

使用策略模式，支持切换不同渲染 API。

### 3. 输入设备 (`neko::device`)

- **Keyboard**：修饰键状态（ctrl/shift/alt）、边沿触发按键检测（`just_pressed`/`just_released`）、字符输入缓冲（16 字符）
- **Mouse**：DPI 感知点击测试——矩形（`is_inside`）、圆形、圆角矩形、多边形（射线法）命中检测；按键边沿检测（`left/right/middle_clicked/released`）

### 4. 响应式组件 (`neko::component`)

- **ValueState\<T\>**：带脏标记绑定的响应式值封装，赋值时自动触发 `mark_dirty()` 回调
- **Animation\<T, EasingFn, TimeType\>**：支持 **12 种速率曲线**的动画引擎（linear、sine、quad、cubic、quart、quint、expo、circ、back、elastic、bounce），每种曲线含 `in`/`out`/`in_out` 三种缓动方向。`AnimationBase` 管理生命周期（`is_active()`、`bind(on_start, on_end)`）

### 5. 引擎核心 (`neko::engine`)

**核心类**：

| 类 | 职责 |
|---|------|
| `Engine` | 总控制器：拥有 Backend、WidgetTree、MsgPump、RenderScheduler、EventRouter、InvalidationTracker、输入设备、Context。提供 `set_root_widget<T>()` 模板工厂、`render_frame()` 帧绘制 |
| `Context` | 引擎共享上下文：`mark_dirty` 回调、Mouse/Keyboard 弱引用、`ColorScheme`、原生窗口句柄 |
| `WidgetTree` | Widget 树管理：root/focus 指针（`std::atomic`）、ID→Widget 索引映射、`next_focus`/`prev_focus` 焦点导航。遍历操作（`for_each`、`render`、`input`、`hit_test`）通过递归 `traverse_impl` 实现。`std::shared_mutex` 线程安全 |
| `EventRouter` | 事件分发：`std::visit` 模式匹配各事件类型 → 分发到设备更新、WidgetTree 输入、主题变更、DPI 调整、调度器、销毁处理 |
| `InvalidationTracker` | 脏标记跟踪：`dirty_` 原子标志 + `animation_` 计数器 + 脏 Widget 列表（`shared_mutex` 保护）。提供 `needs_frame()`、`consume_dirty_list()` |
| `MsgPump` | 线程安全消息队列：有界 SPSC 环形缓冲区（32 槽）+ `std::counting_semaphore<32>` + `std::condition_variable` + `std::jthread` |
| `RenderScheduler` | 独立渲染线程：`std::jthread` + `condition_variable`，帧回调驱动。`request_frame()` 唤醒渲染、`set_pending_size()` 处理 resize |
| `MutableWidget` | 变体容器：`std::variant<monostate, list<MutableWidget>, vector<MutableWidget>, shared_ptr<Widget>>`，支持四种 Widget 子节点组织方式 |

**全局消息流**：

```
WM_* → Platform::translate_event() → MsgPump::push_msg()
  → MsgPump::msg_loop() → EventRouter::dispatch()
    → std::visit → handle_input / handle_resize / handle_dpi_change / handle_theme_change / handle_destroy
      → WidgetTree::input() / RenderScheduler::resize / Context::scheme 更新 / WidgetTree::clear()
```

### 6. Widget 系统 (`neko::widget`)

- **Widget** 基类：虚方法 `draw(Vec4I, Context&, Backend&)`、`build(Context&)`、`event(Context&)`、`input(Context&, Event)`、`hit_test(Mouse)`。Protected 成员：`bounds`（`Vec4I`）、`z_index`（`int32_t`）、`dirty`、`isFocus`
  - **Builder API**（Phase 1）：`build<T>(Args...) -> T&`（创建子 Widget 并链式配置）、`children(fn)`（lambda 作用域批量创建子节点）、`parent()`（获取父 Widget）
  - **子节点存储**：`children_`（`MutableWidget`），通过 `std::visit` + auto-conversion 管理 monostate → shared_ptr → list 的自动转换
  - **StyledWidget\<Derived\>**：CRTP 中间层，提供 `set_background/set_size/set_border` 链式 setter（返回 `Derived&`）
- **Button**：继承 `StyledWidget<Button>`，实现 `draw`（背景/边框/文字居中）、`input`（左键点击→on_click 回调）、`hit_test`。构造函数 `Button(text, on_click)`，链式 setter `on_click()`/`text()`
- **Column**：继承 `StyledWidget<Column>`，垂直布局容器。子节点遍历由 `WidgetTree::traverse_impl` 处理
- **Center**（空存根）：继承 Widget，仅覆盖 `draw()`

### 7. 样式系统 (`neko::style`)

- **ColorScheme**：Material You 36 色调配色方案。包含 **HCT（Hue/Chroma/Tone）色彩引擎**完整实现（sRGB ↔ XYZ ↔ CAM16 ↔ HCT 转换管线）：
  - sRGB 线性化/反线性化
  - D65 XYZ 矩阵变换
  - CAM16 色表模型（Hunt-Pointer-Estevez 矩阵 + 色适应函数）
  - HCT→sRGB 牛顿迭代求解（5 轮 + 16 轮二分 chroma 回退）
  - `light(seed)` / `dark(seed)` 工厂方法生成 36 色调色板
- **CSS 基础结构**：`Background`（Color）、`Size`（size/margin/padding）、`Border`（size/color）

### 线程模型

- **主线程**: Win32 消息循环（`GetMessageW` / `DispatchMessageW`）
- **渲染线程**: `RenderScheduler` 通过 `std::jthread` 驱动帧
- **消息线程**: `MsgPump` 通过 `std::jthread`（有界 SPSC 环形缓冲区 + 信号量）
- Widget 树通过 `std::shared_mutex` 实现线程安全

### 设计模式

| 模式 | 使用位置 |
|------|---------|
| 策略模式 | `Backend` 抽象基类 + `DirectX11` 具体实现 |
| 模板方法 | `Platform` 基类 + `Win32` 具体实现 |
| 单例模式 | `Platform::instance()` |
| 组合模式 | Widget 树形结构与子节点（`MutableWidget` 变体容器） |
| 观察者模式 | `InvalidationTracker` 脏标记 + `AnimationBase` 动画跟踪 + `ValueState` 绑定 |
| 代理模式 | `MsgPump` 作为线程安全事件代理 |
| 工厂方法 | `Engine::set_root_widget<T>()`、`register_platform<T>()` |

## Source Tree

```
NekoUI/                          # Git 仓库根
├── .clang-format                 # C++/JS 代码格式化配置（基于 LLVM 自定义，列宽 220）
├── .gitignore                    # VS 标准忽略 + 项目自定义
├── AGENTS.md                     # 本文档
├── README.md                     # 一行：草稿状态
├── NekoUI.slnx                   # VS 2022+ 解决方案文件
├── NekoUI/                       # 项目目录
│   ├── main.cpp                  # 入口：WinMain、窗口创建、D3D11 初始化、消息循环、Engine 启动
│   ├── NekoUI.vcxproj            # MSBuild 项目文件（14 源文件 + 25 头文件）
│   ├── NekoUI.vcxproj.filters    # VS 虚拟筛选器
│   └── NekoUI/                   # 库头文件与实现
│       ├── NekoUI.hpp            # 主包含头文件（转发 Engine.hpp）
│       ├── Type.hpp              # 核心类型：Vec2/3/4<T>（union xyzw/rgba）、Color（uint32 RGBA）、Handle（void*）
│       ├── Backend/
│       │   ├── Backend.hpp       # 渲染后端抽象基类（策略模式）
│       │   ├── stb_truetype.h    # 嵌入式字体光栅化（4490 行，gitignored — 不跟踪）
│       │   └── DirectX11/
│       │       ├── DirectX11.hpp # D3D11 实现头文件（设备/交换链/着色器/字体图集/CJK 字形）
│       │       └── DirectX11.cpp # D3D11 完整实现（515 行）
│       ├── Component/
│       │   ├── ValueState.hpp    # 响应式值封装 + 脏标记绑定
│       │   └── Animation.hpp     # 12 种速率曲线动画引擎（325 行）
│       ├── Device/
│       │   ├── Keyboard.hpp      # 键盘状态（修饰键 + 边沿检测 + 字符缓冲）
│       │   └── Mouse.hpp         # 鼠标状态 + DPI 感知 4 种命中测试（129 行）
│       ├── Engine/
│       │   ├── Context.hpp       # 引擎共享上下文
│       │   ├── Engine.hpp        # 引擎主类声明
│       │   ├── Engine.cpp        # 引擎实现（70 行）
│       │   ├── EventRouter.hpp   # 事件路由声明
│       │   ├── EventRouter.cpp   # std::visit 事件分发（89 行）
│       │   ├── InvalidationTracker.hpp  # 脏标记跟踪声明
│       │   ├── InvalidationTracker.cpp   # 实现（36 行）
│       │   ├── MsgPump.hpp       # 线程安全消息队列声明
│       │   ├── MsgPump.cpp       # SPSC 环形缓冲区实现（52 行）
│       │   ├── MutableWidget.hpp # 变体 Widget 容器（四种形式）
│       │   ├── RenderScheduler.hpp      # 独立渲染线程声明
│       │   ├── RenderScheduler.cpp       # 实现（56 行）
│       │   └── WidgetTree.hpp/.cpp      # Widget 树管理（焦点导航 + ID 映射）
│       ├── Platform/
│       │   ├── Event.hpp         # 事件类型（变体：9 种事件 + Overloaded 工具）
│       │   ├── Platform.hpp      # 平台基类（单例，含 IME / 11 窗口操作接口）
│       │   └── Win32/
│       │       ├── Win32.hpp     # Win32 平台实现声明（TSF IME 状态）
│       │       └── Win32.cpp     # WM_* → Event 翻译 + 注册表主题检测（168 行）
│       ├── Style/
│       │   ├── ColorScheme.hpp   # Material You 36 色调色板结构体
│       │   ├── ColorScheme.cpp   # HCT 色彩引擎完整实现（354 行）
│       │   └── CSS.hpp           # 基础样式结构体（Background/Size/Border）
│       └── Widget/
│           ├── Widget.hpp        # Widget 基类声明（含 Builder API）
│           ├── Widget.cpp        # Widget 默认实现（13 行）
│           ├── StyledWidget.hpp  # CRTP 中间层（set_background/set_size/set_border）
│           ├── Button/
│           │   ├── Button.hpp    # Button 声明（空存根）
│           │   └── Button.cpp    # Button 空实现（16 行）
│           ├── Layout/
│               ├── Center.hpp    # Center 布局声明（空存根）
│               ├── Center.cpp    # Center 空实现（4 行）
│               ├── Column.hpp    # Column 垂直布局声明
│               └── Column.cpp    # Column 实现（draw/hit_test）
```

## Build

- **构建命令**: 在 Visual Studio 2022+ 中打开 `NekoUI.slnx` 并构建，或通过命令行：
  ```bat
  msbuild NekoUI.slnx
  ```

### 构建配置

| 配置 | 平台 | 工具集 | 语言标准 | 运行时库 | 备注 |
|------|------|--------|---------|---------|------|
| Debug | Win32 | v145 (MSVC) | C++20 | /MDd | SDL 检查开启 |
| Release | Win32 | v145 (MSVC) | C++20 | /MD | 全程序优化 |
| Debug | x64 | Intel C++ Compiler 2026 | C++latest | /MTd | TBB/IPP(Static)/MKL(Parallel)/DAL/MPI、ARROWLAKE-S、Async 异常 |
| Release | x64 | Intel C++ Compiler 2026 | C++latest | /MT | 同上 + PGO Instrumentation、CFG Guard、MaxSpeedHighLevel |

- **输出目录**: `$(SolutionDir)$(Platform)-$(Configuration)/`（如 `x64-Debug/`）
- **中间目录**: `$(SolutionDir)$(Platform)-$(Configuration)/.tmep/`
- **子系统**: 控制台（Console）
- **DPI 感知**: PerMonitorHighDPIAware (x64)
- **注意**: x64 配置使用 Intel C++ Compiler 2026，需要在开发机上安装 Intel oneAPI 2026 工具集；Win32 配置已声明 `roapi.lib` 依赖项已移除（主题色改用注册表读取）

## Test

当前无测试框架或测试目录。

## Conventions

### 编码风格

遵循 `.clang-format` 配置（基于 LLVM 自定义）：

- **缩进**: 4 空格，Tab 宽度 4
- **列限制**: 220 字符
- **花括号**: 不换行（类、函数、控制语句、命名空间之后均不换行）
- **指针对齐**: 左对齐（`int* p`）
- **访问修饰符偏移**: -4（缩进 4 格）
- **构造函数初始化器**: 冒号后在列首断行
- **模板声明**: 强制断行
- **命名空间**: 缩进全部内容
- **Include 优先级**: `<...>` 优先（优先级 1），`"..."` 其次（优先级 2）
- **Include 主文件**: 匹配 `([-_](test|unittest))?$`

### 命名规范

| 元素 | 风格 | 示例 |
|------|------|------|
| 命名空间 | snake_case | `neko::engine`、`neko::widget` |
| 类型（class/struct/enum） | PascalCase | `Engine`、`Widget`、`DirectX11`、`RenderScheduler` |
| 函数/方法 | snake_case + 尾置返回类型 | `auto get_name() -> std::string` |
| 成员变量 | snake_case | `mark_dirty`、`bounds`、`children_`（私有成员尾随 `_`） |
| 参数 | snake_case | `user_id`、`target` |
| 枚举器 | PascalCase | `MouseButton::Left` |
| 宏 | UPPER_SNAKE_CASE | `NOMINMAX`、`WINDOWS_API` |

### 代码规范

- **头文件防护**: `#pragma once` 统一使用
- **尾置返回类型**: 统一使用 `auto func() -> ReturnType` 语法
- **现代 C++**: 使用 C++20/C++latest 特性，如 `std::optional`、`std::span`、`std::jthread`、`std::shared_mutex`、`std::atomic`、`constexpr`、`requires` 约束、`[[nodiscard]]`、设计初始化器（`.x = value`）
- **智能指针**: 优先 `std::unique_ptr` / `std::shared_ptr`，避免裸指针所有权
- **标准数组**: 优先 `std::array<T, N>` 替代 C 样式数组 `T[N]`，提供边界安全与值语义
- **数学常量**: 优先使用 `std::numbers` 中的常量（如 `std::numbers::pi`），禁止重复定义标准数学常量
- **数值边界**: 使用 `std::numeric_limits<T>` 获取类型的数值上下限，禁止硬编码魔法数值
- **删除拷贝语义**: 不可拷贝的类显式 `= delete` 拷贝构造/赋值
- **异常处理**: 使用异常（`try`/`catch`），Async 异常处理模型
- **Lambda**: 使用现代 C++ lambda，优先值捕获或显式引用捕获

### 样式细节

- RTTI 已启用（x64 Debug 和 Release 均显式设置 `RuntimeTypeInfo=true`，Win32 走 MSVC 默认启用）
- SDL 检查在 Win32 配置开启，x64 Debug 关闭
- x64 配置启用 ARROWLAKE-S 指令集及替代代码路径
- Win32 配置使用 D3D11_CREATE_DEVICE_DEBUG（Debug 下）
- 浮点模型: Precise（精确）
- 异常处理模型: Async

### 文件结构

- `.hpp` 头文件 + `.cpp` 实现文件
- 目录按功能域组织（`Backend/`、`Engine/`、`Platform/`、`Widget/`、`Component/`、`Device/`、`Style/`）
- Include 路径使用相对路径（`../Type.hpp`、`../Backend/Backend.hpp`）

## Dependencies

| 依赖 | 用途 | 版本/来源 |
|------|------|----------|
| DirectX 11 (`d3d11.h`) | GPU 渲染 API | Windows SDK |
| DXGI (`dxgi1_2.h`) | 交换链管理 | Windows SDK |
| DirectXMath | 数学运算 | Windows SDK |
| Windows SDK | Win32 API + 注册表 API | 系统组件 |
| Intel oneAPI 2026（仅 x64） | TBB（并行）、IPP（图像处理）、MKL（数学）、DAL（数据分析）、MPI | Intel oneAPI 2026 |
| stb_truetype.h | 字体光栅化（编译期包含，gitignored — 不跟踪版本） | 项目内嵌（Backend/stb_truetype.h） |
| MSVC v143/v145 | C++ 标准库 | Visual Studio 2022 |

**无**外部包管理器（无 vcpkg、无 NuGet、无 npm）— 所有依赖均通过 Windows SDK 和 Visual Studio/Intel 工具集解析。

## Current Status

- **草稿状态** — 核心架构已建立，部分功能尚不完整
- **已实现**：跨平台渲染后端抽象（D3D11 实现）、HCT Material You 色彩引擎（sRGB→XYZ→CAM16→HCT 完整管线）、平台抽象（IME TSF / 11 窗口操作 / 注册表主题检测 + 强调色）、响应式 Widget 树（含焦点导航）、Widget Builder API（`build<T>()` / `children()` / `StyledWidget` CRTP）、12 种速率曲线动画引擎、线程安全事件传递、系统主题变化检测与传递（Light/Dark + AccentColor）
- **未实现**：`Button` 类仅空存根（声明完整但实现为空）、`Layout/Center` 仅空存根（仅 `draw()` 空实现）、无可运行的测试
- **主题色获取**：使用注册表 `HKCU\...\Windows\DWM\AccentColor`（ABGR → RGBA 转换），不再使用已过时的 `DwmGetColorizationColor`
