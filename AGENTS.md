# NekoUI

## Overview

NekoUI 是一个 Windows C++ GUI 框架（UI 库），使用 DirectX 11 渲染。处于开发早期（草稿状态）。核心架构包括跨平台渲染后端抽象、响应式 Widget 树、速率曲线动画引擎、独立渲染线程和线程安全消息队列。

- **语言标准**: C++20 (Win32) / C++latest (x64)
- **渲染**: DirectX 11
- **平台**: Windows (Win32 API)
- **命名空间**: `neko`，子命名空间包括 `neko::type`、`neko::engine`、`neko::engine::internal`、`neko::widget`、`neko::backend`、`neko::platform`、`neko::device`、`neko::component`、`neko::style`

## Architecture

分层架构，从底层到上层依次为：

1. **平台抽象层** (`neko::platform`) — 平台事件封装与转换。`Platform` 基类（单例）负责将原生消息（如 Win32 WM\_\*）转换为统一 `Event` 变体。支持的事件包括：鼠标/键盘输入、窗口尺寸变化、DPI 变化、窗口销毁（`DestroyEvent`，`WM_DESTROY`）、系统主题切换（`ThemeChangedEvent`，`WM_SETTINGCHANGE/ImmersiveColorSet` + 注册表/DWM 查询）。主题数据以 `style::ColorScheme` 形式存储在 `Context::scheme` 中，Widget 可直接读取各色彩角色。此外，`Platform` 提供 IME 输入法控制（TSF 接口）以及 11 个窗口操作方法（显示/隐藏/关闭/最大化/最小化/还原/移动/调整大小/设置焦点/设置不透明度）。使用 `register_platform<T>()` 工厂方法 + `NEKO_REGISTER_PLATFORM(T)` 宏注册具体实现。
2. **渲染后端** (`neko::backend`) — `Backend` 抽象基类定义绘制接口（矩形、圆形、线条、文本），`DirectX11` 为其具体实现。使用策略模式，支持切换不同渲染 API。
3. **输入设备** (`neko::device`) — 键盘/鼠标状态管理。`Keyboard` 支持修饰键状态（Ctrl/Shift/Alt）、边沿触发按键检测、字符输入缓冲。`Mouse` 支持 DPI 感知点击测试，包括矩形、圆形、圆角矩形和多边形（射线法）命中检测。
4. **响应式组件** (`neko::component`) — 带脏标记绑定的 `ValueState` 响应式值封装，以及支持 12 种速率曲线（linear/sine/quad/cubic/quart/quint/expo/circ/back/elastic/bounce）的 `Animation` 引擎。
5. **引擎核心** (`neko::engine`) — 渲染帧管理、Widget 树（根/焦点/ID 映射）、事件路由、脏区域/动画跟踪、线程安全消息队列（`MsgPump` 有界 SPSC 环形缓冲区+信号量）、独立渲染线程（`RenderScheduler` 基于 `std::jthread` + 条件变量）。
6. **Widget 系统** (`neko::widget`) — `Widget` 基类（约束、绘制、布局、事件、子节点、命中测试），`MutableWidget` 变体容器（支持单 Widget/列表/向量/弱引用四种形式），`WidgetTree` 管理根/焦点/ID 映射及焦点导航。具体 Widget：`Button`（空存根，类尚未实现），`Center` 布局（空存根）。

### 线程模型

- **主线程**: Win32 消息循环 (`GetMessageW` / `DispatchMessageW`)
- **渲染线程**: `RenderScheduler` 通过 `std::jthread` 驱动帧
- **消息线程**: `MsgPump` 通过 `std::jthread`（有界 SPSC 环形缓冲区 + 信号量）
- Widget 树通过 `std::shared_mutex` 实现线程安全

### 设计模式

| 模式       | 使用位置                                                |
| ---------- | ------------------------------------------------------- |
| 策略模式   | `Backend` 抽象基类 + `DirectX11` 具体实现               |
| 模板方法   | `Platform` 基类 + `Win32` 具体实现                      |
| 单例模式   | `Platform::instance()`                                  |
| 组合模式   | Widget 树形结构与子节点                                 |
| 观察者模式 | `InvalidationTracker` 脏标记 + `AnimationBase` 动画跟踪 |
| 代理模式   | `MsgPump` 作为线程安全事件代理                          |
| 工厂方法   | `Engine::set_root_widget<T>()`、`register_platform<T>()` |

## Source Tree

```
NekoUI/                          # Git 仓库根
├── .clang-format                 # C++ 代码格式化配置
├── .gitignore
├── NekoUI.slnx                   # VS 2022+ 解决方案文件
├── README.md
├── AGENTS.md                     # 本文档
├── NekoUI/                       # 项目目录（所有源码）
│   ├── main.cpp                  # 入口点：WinMain、窗口创建、D3D11 初始化、消息循环
│   ├── NekoUI.vcxproj            # MSBuild 项目文件
│   └── NekoUI/                   # 库头文件与实现
│       ├── NekoUI.hpp            # 主包含头文件（转发 Engine.hpp）
│       ├── Type.hpp              # 核心类型：Vec2/3/4<T>、Color、Handle
│       ├── Backend/              # 渲染后端抽象层
│       │   ├── Backend.hpp       # 抽象基类
│       │   ├── stb_truetype.h    # 字体光栅化（嵌入，5079 行）
│       │   └── DirectX11/        # DirectX 11 实现
│       ├── Component/            # 响应式/动画组件
│       │   ├── ValueState.hpp    # 响应式值封装
│       │   └── Animation.hpp     # 速率曲线动画引擎
│       ├── Device/               # 输入设备
│       │   ├── Keyboard.hpp      # 键盘状态
│       │   └── Mouse.hpp         # 鼠标状态（含 DPI 感知命中测试）
│       ├── Engine/               # 渲染引擎核心
│       │   ├── Context.hpp       # 引擎上下文
│       │   ├── Engine.hpp/.cpp   # 引擎主类
│       │   ├── EventRouter.hpp/.cpp   # 事件分发
│       │   ├── InvalidationTracker.hpp/.cpp # 脏区域/动画跟踪
│       │   ├── MsgPump.hpp/.cpp  # 线程安全消息队列
│       │   ├── MutableWidget.hpp # 可变 Widget 容器
│       │   ├── RenderScheduler.hpp/.cpp # 独立渲染线程
│       │   └── WidgetTree.hpp/.cpp      # Widget 树管理（含焦点导航）
│       ├── Platform/             # 平台抽象层
│       │   ├── Event.hpp         # 事件类型（变体）
│       │   ├── Platform.hpp      # 平台基类（单例，含 IME/窗口操作接口）
│       │   └── Win32/            # Win32 平台实现
│       ├── Style/                # 样式系统
│       │   ├── ColorScheme.hpp/.cpp # Material You 36 色调配色方案（HSL 近似，light/dark 双模式）
│       │   └── CSS.hpp           # 基础样式结构体（Background/Size/Border）
│       └── Widget/               # Widget 系统
│           ├── Widget.hpp/.cpp   # Widget 基类
│           ├── Button/           # Button Widget（空存根，类尚未实现）
│           │   ├── Button.hpp
│           │   └── Button.cpp
│           └── Layout/           # 布局 Widget
│               └── Center.hpp    # 居中布局（空存根，仅 #pragma once）
```

## Build

- **构建命令**: 在 Visual Studio 2022+ 中打开 `NekoUI.slnx` 并构建，或通过命令行：
  ```
  msbuild NekoUI.slnx
  ```

### 构建配置

| 配置    | 平台  | 工具集                  | 语言标准  | 备注                                                  |
| ------- | ----- | ----------------------- | --------- | ----------------------------------------------------- |
| Debug   | Win32 | v145 (MSVC)             | C++20     | 标准调试                                              |
| Release | Win32 | v145 (MSVC)             | C++20     | 全程序优化                                            |
| Debug   | x64   | Intel C++ Compiler 2026 | C++latest | Intel oneAPI 全套（TBB/IPP/MKL/DAL/MPI），ARROWLAKE-S |
| Release | x64   | Intel C++ Compiler 2026 | C++latest | 同上 + PGO Instrumentation                            |

- **输出目录**: `$(SolutionDir)$(Platform)-$(Configuration)/`（如 `x64-Debug/`）
- **中间目录**: `$(SolutionDir)$(Platform)-$(Configuration)/.tmep/`
- **子系统**: 控制台
- **注意**: x64 配置使用 Intel C++ Compiler 2026，需要在开发机上安装 Intel oneAPI 2026 工具集

## Test

当前无测试框架或测试目录。

## Conventions

### 编码风格

遵循 `.clang-format` 配置（基于 LLVM 自定义）：

- **缩进**: 4 空格，Tab 宽度 4
- **列限制**: 220 字符
- **花括号**: 不换行（类、函数、控制语句、命名空间之后均不换行）
- **指针对齐**: 左对齐
- **访问修饰符偏移**: -4（缩进 4 格）
- **构造函数初始化器**: 冒号后在列首断行
- **模板声明**: 强制断行
- **命名空间**: 缩进全部内容
- **Include 优先级**: `<...>` 优先（优先级 1），`"..."` 其次（优先级 2）
- **Include 主文件**: 匹配 `([-_](test|unittest))?$`

### 命名规范

| 元素                      | 风格                      | 示例                                                    |
| ------------------------- | ------------------------- | ------------------------------------------------------- |
| 命名空间                  | snake_case                | `neko::engine`、`neko::widget`                          |
| 类型（class/struct/enum） | PascalCase                | `Engine`、`Widget`、`DirectX11`、`RenderScheduler`      |
| 函数/方法                 | snake_case + 尾置返回类型 | `auto get_name() -> std::string`                        |
| 成员变量                  | snake_case                | `mark_dirty`、`bounds`、`children_`（私有成员尾随 `_`） |
| 参数                      | snake_case                | `user_id`、`target`                                     |
| 枚举器                    | PascalCase                | `MouseButton::Left`                                     |
| 宏                        | UPPER_SNAKE_CASE          | `NOMINMAX`、`WINDOWS_API`                               |

### 代码规范

- **头文件防护**: `#pragma once` 统一使用
- **尾置返回类型**: 统一使用 `auto func() -> ReturnType` 语法
- **现代 C++**: 使用 C++20/C++latest 特性，如 `std::optional`、`std::span`、`std::jthread`、`std::shared_mutex`、`std::atomic`、`constexpr`、`requires` 约束、`[[nodiscard]]`、设计初始化器（`.x = value`）
- **智能指针**: 优先 `std::unique_ptr` / `std::shared_ptr`，避免裸指针所有权
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

- `.hpp` 头文件 + `(.cpp)` 实现文件
- 目录按功能域组织（`Backend/`、`Engine/`、`Platform/`、`Widget/`、`Component/`）
- Include 路径使用相对路径（`../Type.hpp`、`../Backend/Backend.hpp`）

## Dependencies

| 依赖                        | 用途                                                            | 版本/来源                          |
| --------------------------- | --------------------------------------------------------------- | ---------------------------------- |
| DirectX 11 (`d3d11.h`)      | GPU 渲染 API                                                    | Windows SDK                        |
| DXGI (`dxgi1_2.h`)          | 交换链管理                                                      | Windows SDK                        |
| DirectXMath                 | 数学运算                                                        | Windows SDK                        |
| Windows SDK                 | Win32 API                                                       | 系统组件                           |
| Intel oneAPI 2026（仅 x64） | TBB（并行）、IPP（图像处理）、MKL（数学）、DAL（数据分析）、MPI | Intel oneAPI 2026                  |
| stb_truetype.h              | 字体光栅化（编译期包含）                                        | 项目内嵌（Backend/stb_truetype.h） |
| MSVC v143/v145              | C++ 标准库                                                      | Visual Studio 2022                 |

**无**外部包管理器（无 vcpkg、无 NuGet、无 npm）— 所有依赖均通过 Windows SDK 和 Visual Studio/Intel 工具集解析。

## Current Status

- **草稿状态** — 核心架构已建立，功能尚不完整
- 已实现：跨平台渲染后端抽象、平台抽象（IME/窗口操作）、响应式 Widget 树（含焦点导航）、速率曲线动画引擎、线程安全事件传递、窗口销毁事件处理、系统主题变化检测与传递、Material You 36 色调色板
- 未实现：`Layout/Center.hpp` 为空、`Button` 类尚未实现（仅空存根）、无可运行的测试
- **注意**：`main.cpp` 引用了 `neko::widget::Button` 类，但 `Button` 并未定义——当前代码不可编译
