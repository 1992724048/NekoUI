# NekoUI

## Overview

NekoUI 是一个基于 DirectX 11 + Win32 API 的自研 C++ 桌面 UI 框架，定位为轻量级、硬件加速的 GUI 引擎。包含自研渲染后端、控件体系、动画引擎和输入系统。当前处于早期开发阶段。

## Architecture

### 总体架构: Retained Mode UI + 双线程流水线

```
Engine (引擎)
  ├── owns → Backend (渲染后端抽象)
  ├── owns → Context (依赖注入容器)
  ├── owns → Mouse (鼠标输入)
  ├── owns → Keyboard (键盘输入)
  ├── owns → WidgetTree (控件树: root/focused/id_widgets)
  ├── owns → InvalidationTracker (脏标记 + 动画计数器)
  ├── owns → RenderScheduler (渲染调度: render_thread + cv 等待 + resize 状态)
  ├── owns → MsgPump (消息泵: msg_thread + 环形队列 + counting_semaphore)
  └── owns → EventRouter (事件路由: Win32 消息 → Widget 树)
```

- **消息线程**: 由 `MsgPump` 管理，处理 Win32 消息 → `EventRouter::dispatch()`（mouse/keyboard 状态机 + Widget 树路由 + dirty 检查 → notify 渲染）
- **渲染线程**: 由 `RenderScheduler` 管理，等待 notify → 回调 `Engine::render_frame()`（layout + draw + Present）
- **Widget 树**: 单根结构，Engine 通过 `set_root_widget<T>()` 设置根 Widget（委托 `WidgetTree` 管理 root/focused/id 注册表）

### 核心模块

| 模块                | 路径                                   | 说明                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| ------------------- | -------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Engine              | `Engine/Engine.hpp/.cpp`               | 双线程引擎，消息+渲染分离；`set_root_widget<T>()` 返回 `shared_ptr<T>`（委托 `widget_tree_.set_root()`）；持有 `WidgetTree` + `InvalidationTracker` + `RenderScheduler` + `MsgPump` + `EventRouter` 组件（纯组合，无继承）；Context 回调绑定 widget*tree*/invalidation\_；`render_frame()` 作为回调注入 RenderScheduler（依赖 Backend + WidgetTree）；`EventRouter::dispatch()` 作为回调注入 MsgPump；持有 `device::Mouse/Keyboard` 共享指针 |
| WidgetTree          | `Engine/WidgetTree.hpp/.cpp`           | 控件树管理：`root_` (atomic<shared*ptr<Widget>>) + `focused*`(atomic<weak_ptr<Widget>>) +`id*widgets*` (map<string, weak_ptr> + shared_mutex)；set_root/get_root/set_focus/get_focus/register_widget/unregister_widget/clear                                                                                                                                                                                                                 |
| InvalidationTracker | `Engine/InvalidationTracker.hpp/.cpp`  | 脏标记 + 动画计数器：dirty* flag + animation* counter + dirty*list* (shared_mutex)；mark_dirty/mark_widget_dirty/anim_inc/anim_dec/is_dirty/needs_frame/clear                                                                                                                                                                                                                                                                                |
| RenderScheduler     | `Engine/RenderScheduler.hpp/.cpp`      | 渲染调度：render*thread* (std::jthread) + render*notify* (cv) + render*mutex* + pending* (atomic_bool) + resize 状态 (resize_pending* + pending*width*/height\_ atomic)；request_frame/set_pending_size/stop/consume_resize/pending_size；render_loop/render_wait 内部线程函数；FrameCallback 回调注入（Engine::render_frame）                                                                                                               |
| MsgPump             | `Engine/MsgPump.hpp/.cpp`              | 消息泵：msg*thread* (std::jthread) + 环形队列 (kQueueSize=32, std::array) + msg*notify* (cv) + msg*space* (counting*semaphore) + running* (atomic_bool)；push_msg/stop/msg_loop/msg_dequeue；MessageHandler 回调注入（Engine lambda 调用 `EventRouter::dispatch()`）                                                                                                                                                                         |
| EventRouter         | `Engine/EventRouter.hpp/.cpp`          | 事件路由：`platform::Event` variant → Widget 树；持有 7 个引用 (WidgetTree/Mouse/Keyboard/Context/Backend/RenderScheduler/InvalidationTracker)；dispatch() 通过 std::visit 按事件类型分发到 Mouse/Keyboard 的 per-type handle() 重载 + resize/dpi 处理 + dirty 检查 → request_frame                                                                                                                                                             |
| Context             | `Engine/Context.hpp`                   | 轻量级 DI 容器（callback + 弱引用）；包含对 Backend + Widget 的前向声明                                                                                                                                                                                                                                                                                                                                                                      |
| MutableWidget       | `Engine/MutableWidget.hpp`             | `std::variant<std::monostate, bool, int, float, double, std::string, std::weak_ptr<widget::Widget>>` — 动态属性值类型，支持 Widget 弱引用存储                                                                                                                                                                                                                                                                                                |
| Backend             | `Backend/Backend.hpp`                  | 纯抽象渲染后端接口；纯虚方法：resize/set_dpi/get_dpi_scale/begin/end/draw_rect_fill/draw_rect/draw_line/draw_circle_fill/draw_text                                                                                                                                                                                                                                                                                                                                                                     |
| DirectX11           | `Backend/DirectX11/DirectX11.hpp/.cpp` | DX11 具体实现；继承 Backend 纯抽象接口，实现全部渲染方法；构造函数 `(ID3D11Device*, ID3D11DeviceContext*, HWND)` 外部注入 Device/Context                                                                                                                                                                                                                                                                                                                                                                                                          |
| Widget              | `Widget/Widget.hpp/.cpp`               | 控件基类，虚方法体系；`Constraints` 布局结构体 (x/y/width/height)，`add<T>()` 子控件工厂，`raw_event()`/`hit_test()` 虚方法；线程安全 children 列表 (shared_mutex)                                                                                                                                                                                                                                                                           |
| Button              | `Widget/Button/Button.hpp/.cpp`        | 当前唯一具体控件                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Animation           | `Component/Animation.hpp`              | 缓动动画引擎：AnimationBase 多态基类 + Animation<T,auto EasingFnType,TimeType> 模板值动画；EasingFn concept 约束；6 组缓动子命名空间（30+ constexpr 函数）                                                                                                                                                                                                                                                                                   |
| ValueState          | `Component/ValueState.hpp`             | 响应式状态包装器                                                                                                                                                                                                                                                                                                                                                                                                                             |
| Event               | `Platform/Event.hpp`                   | 统一事件类型聚合：包含 device:: 层事件（MouseMoveEvent/ButtonEvent/WheelEvent、KeyEvent/CharEvent）+ platform:: 层事件（ResizeEvent/DpiChangeEvent）；定义 `platform::Event` variant + `Overloaded` visitor 辅助                                                                                                                                                                                                                              |
| Mouse               | `Device/Mouse.hpp`                     | 鼠标输入状态机；定义 MouseMoveEvent/MouseButtonEvent/MouseWheelEvent 事件类型；每个事件类型有独立的 `handle()` 重载更新状态；4 种 hit-test 几何                                                                                                                                                                                                                                                                                             |
| Keyboard            | `Device/Keyboard.hpp`                  | 键盘输入状态机；定义 KeyEvent/CharEvent 事件类型；每个事件类型有独立的 `handle()` 重载更新状态                                                                                                                                                                                                                                                                                                                                              |
| Platform            | `Platform/Platform.hpp`                | 平台抽象基类：`translate_event(const NativeMessage&)` 纯虚方法将平台原生消息翻译为 `platform::Event` variant；`NativeMessage` 为不透明类型，由各平台实现定义                                                                                                                                                                                                                                                                                  |
| Win32               | `Platform/Win32/Win32.hpp/.cpp`        | Win32 平台实现：定义 `NativeMessage = {UINT msg, WPARAM wparam, LPARAM lparam}`；`Win32::translate_event()` 将 Win32 消息（WM_MOUSEMOVE/WM_KEYDOWN/WM_SIZE 等）翻译为对应的 device:: / platform:: 事件                                                                                                                                                                                                                                      |

## Source Tree

```
D:\CODE\NekoUI\
├── .clang-format
├── AGENTS.md
├── README.md
├── R#.DotSettings                 # ReSharper 配置
├── NekoUI.slnx                    # VS 解决方案
├── NekoUI/
│   ├── main.cpp                   # 应用入口
│   ├── NekoUI.vcxproj             # 项目配置
│   └── NekoUI/                    # 核心源码
│       ├── NekoUI.hpp             # 统一入口头文件 (仅 #include Engine/Engine.hpp)
│       ├── Type.hpp               # 自定义类型 (Vec<T,N> 模板特化 + Color 等)
│       ├── Engine/
│       │   ├── Engine.hpp/.cpp
│       │   ├── EventRouter.hpp/.cpp
│       │   ├── Context.hpp
│       │   ├── InvalidationTracker.hpp/.cpp
│       │   ├── MsgPump.hpp/.cpp
│       │   ├── MutableWidget.hpp
│       │   ├── RenderScheduler.hpp/.cpp
│       │   └── WidgetTree.hpp/.cpp
│       ├── Backend/
│       │   ├── Backend.hpp
│       │   ├── Backend.cpp (empty)
│       │   ├── DirectX11/DirectX11.hpp/.cpp
│       │   └── stb_truetype.h
│       ├── Component/
│       │   ├── Animation.hpp
│       │   └── ValueState.hpp
│       ├── Device/
│       │   ├── Mouse.hpp
│       │   └── Keyboard.hpp
│       ├── Platform/
│       │   ├── Event.hpp
│       │   ├── Platform.hpp
│       │   └── Win32/Win32.hpp/.cpp
│       ├── Style/
│       │   └── CSS.hpp               # CSS 样式结构
│       └── Widget/
│           ├── Widget.hpp/.cpp
│           ├── Button/Button.hpp/.cpp
│           └── Layout/Center.hpp      # (空 — 规划中)
```

## Build

- **平台 x64**: Intel C++ Compiler 2026 (工具集 v145 MSVC 用于 Win32)
- **语言标准**: C++latest (C++23+)
- **指令集 x64 Debug**: `ARROWLAKE-S`
- **指令集 x64 Release**: `ARROWLAKE-S`
- **运行时库**: 静态链接 `/MT`（Release）/ `/MTd`（Debug）
- **PGO**: Release|x64 启用 `ProfileGuidedOptimization=PGOInstrument`
- **输出**: `x64-Debug/` / `x64-Release/`
- **子系统**: Console, PerMonitorHighDPIAware, Segment Heap
- **Intel 加速**: oneTBB / IPP / MKL / DAL / MPI 全部启用
- **TMP/TEMP**: 必须指向 RAM disk 子目录（如 `A:\Temp\tmp`），不能指向 RAM disk 根目录（Intel 编译器的 `tmpfile()` 无法在 RAM disk 根目录创建临时文件）。当前配置为系统级 TMP 指向 `A:\Temp\tmp`，`C:\ProgramData\Temp` 为同名 junction。

```bash
# VS 中: Debug|x64 或 Release|x64
# 命令行:
msbuild NekoUI.slnx /p:Configuration=Debug /p:Platform=x64
```

## Test

无测试项目（工程处于早期阶段）。

## Conventions

### 编码规范

- **格式**: LLVM 风格，4 空格缩进，列宽 220，指针左对齐，namespace 缩进 (`.clang-format`)
- **命名**: 类/struct PascalCase；函数/方法 snake_case 后缀 `_returntype`；namespace snake_case
- **C++ 特性**: 积极使用 C++23 特性 (std::jthread, std::format/println, consteval, requires, std::span, std::optional)
- **内存**: unique_ptr (Engine 拥有 Backend/Context)，shared_ptr (Widget 树)，weak_ptr (parent)
- **自定义类型**: Type.hpp 用 `Vec<T, N>` 模板偏特化（N=2/3/4）+ using 别名替代 glm；Vec<T,3/4> 用匿名 union 支持 .xyzw/.rgba 双访问；Vec<T,2> 支持 operator-/==，其余支持 operator==（C++20 defaulted）；Color 为 uint32_t 封装（0xRRGGBBAA 格式，4 字节），r()/g()/b()/a() constexpr 位移提取分量
- **异常**: 使用 `try` 函数签名包裹 main
- **跨平台**: 当前 Windows-only (Win32 API + DirectX)
- **ReSharper**: R#.DotSettings 配置存在；项目无 `.clang-tidy` 配置文件（clang-tidy 规则通过 ReSharper 集成管理）

### 命名空间结构

| 命名空间          | 目录         | 内容                                                                                                   |
| ----------------- | ------------ | ------------------------------------------------------------------------------------------------------ |
| `neko::type`      | `Type.hpp`   | Vec 系列别名、Color                                                                                    |
| `neko::engine`    | `Engine/`    | Engine, EventRouter, Context, InvalidationTracker, MsgPump, MutableWidget, RenderScheduler, WidgetTree |
| `neko::backend`   | `Backend/`   | Backend 纯抽象接口 + DirectX11 具体实现                                                                 |
| `neko::device`    | `Device/`    | Mouse, Keyboard 输入设备                                                                               |
| `neko::widget`    | `Widget/`    | Widget 基类, Button 等具体控件, MutableWidget(partial)                                                 |
| `neko::component` | `Component/` | Animation 缓动引擎 + ValueState 响应式状态                                                             |
| `neko::platform`  | `Platform/`  | 平台抽象（开发中）                                                                                     |
| `neko::style`     | `Style/`     | CSS 样式结构                                                                                           |

### 动画系统

- `EasingFn` concept — 使用 `std::is_invocable_r_v<float, Fn, float>` 约束缓动函数签名，编译期保证 `float(float)` 可调用性
- 缓动函数以子命名空间组织：`ease::quad::in` / `ease::quad::out` / `ease::quad::in_out`
- `ease::linear` 为 `inline namespace`，可 `ease::in` 简写
- `Animation<T, auto EasingFnType = ease::linear::in, TimeType = milliseconds>` 继承自 `AnimationBase`，`EasingFnType` 为非类型模板参数（函数指针/lambda），编译期零开销，需满足 `EasingFn` 概念约束
- `AnimationBase` 提供 `change_` / `inc_` / `dec_` 跨线程基类设施；`start_` 在派生 `Animation` 模板中

### MutableWidget 变体类型

`MutableWidget` 定义在 `Engine/MutableWidget.hpp`（`neko::engine` 命名空间），是一个基于 `std::variant` 的动态属性值类型：

```cpp
using MutableWidget = std::variant<
    std::monostate,
    bool, int, float, double, std::string,
    std::weak_ptr<widget::Widget>
>;
```

- Widget 替代项使用 `std::weak_ptr<widget::Widget>` 避免自引用递归
- 通过 `is_widget()` / `as_widget()` 访问器安全访问 Widget 弱引用

## Memory (mnemosyne)

- At the start of a session, use memory_recall and memory_recall_global to search for context relevant to the user's first message.
- After significant decisions, use memory_store to save a concise summary.
- Delete contradicted memories with memory_delete before storing updated ones.
- Use memory_recall_global / memory_store_global for cross-project preferences.
- Mark critical, always-relevant context as core (core=true) — but use sparingly.
- When you are done with a session, store any memories that you think are relevant to the user and the project.

## Dependencies

| 依赖           | 方式                          | 用途                                                        |
| -------------- | ----------------------------- | ----------------------------------------------------------- |
| DirectX 11     | `#pragma comment(lib, "...")` | GPU 渲染管线 (`d3d11.lib` / `dxgi.lib` / `d3dcompiler.lib`) |
| stb_truetype.h | 单头文件源码嵌入              | 字体解析/光栅化                                             |
| Win32 API      | Windows.h, dcomp.h            | 窗口管理、消息泵                                            |

## Existing Widget Components

| Widget        | 文件                            | 状态                                                                   |
| ------------- | ------------------------------- | ---------------------------------------------------------------------- |
| Widget (基类) | `Widget/Widget.hpp/.cpp`        | 接口定义完整，draw/layout/raw_event/hit_test 虚方法，线程安全 children |
| Button        | `Widget/Button/Button.hpp/.cpp` | draw/layout/update 全部空实现                                          |
| Center (布局) | `Widget/Layout/Center.hpp`      | 仅声明，未接入                                                         |
| CSS           | `Style/CSS.hpp`                 | 仅声明，未接入                                                         |
