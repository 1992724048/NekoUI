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

| 类                    | 职责                                                                                                                                                                                                                                                                                     |
| --------------------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `Engine`              | 总控制器：拥有 Backend、TreeManager、WidgetBuilder、HitTester、MsgPump、RenderScheduler、EventRouter、InvalidationTracker、输入设备、Context。构造函数通过 std::bind 将所有子系统回调连接起来。提供 `set_root_widget<T>()` 模板工厂（内联构建）、`render_frame()` 帧绘制、`rebuild()` 重建（保留，外部调用入口）、`schedule_rebuild()` 延迟重建（`tree_dirty_` 标志 + `request_frame`，帧首合并）、私有 `draw_widget()` 集中式前序 DFS 递归绘制 |
| `Context`             | 引擎共享上下文：`mark_dirty`/`widget_dirty`/`anim_inc`/`anim_dec`/`widget_tree_changed` 回调、Mouse/Keyboard 弱引用、`tree_mutex`（指向 `TreeManager::mutex_`，树结构锁）、`root`（弱引用根 Widget）、`ColorScheme`、`native_handle`                                                                                                           |
| `TreeManager`         | Widget 树管理：root/focus 原子指针（`std::atomic<shared_ptr<Widget>>`）、ID→Widget 映射、index→Widget 映射、`next_focus`/`prev_focus` 焦点导航。`register_widget()` 分配 z_index 和路径并调用 `build()`。`std::shared_mutex` 线程安全                                                    |
| `Renderer`            | 已删除。渲染驱动并入 `Engine::render_frame`（Engine.cpp:105-139）：consume_resize → 帧首合并重建（`tree_dirty_`）→ 尺寸兜底（`GetClientRect`）→ `root->layout()`（布局阶段已接通）→ `backend->begin()` → `draw_widget` 前序 DFS 递归绘制 → `backend->end()` → `invalidation_.clear()` |
| `HitTester`           | 命中测试器：递归反向遍历子节点（z 顺序），返回 `std::optional<std::shared_ptr<widget::Widget>>`（顶层命中的 Widget），持有 `TreeManager&` 引用 |
| `WidgetBuilder`       | 构建遍历器：递归注册 Widget 到 TreeManager 的 ID/index 映射，调用每个 Widget 的 `build()`。`Engine::schedule_rebuild()` 置标志、帧首合并触发重建 |
| `WidgetVisitor`       | 工具模板：`visit_children()` 统一分发 `MutableWidget` 四种变体（monostate/shared_ptr/list/vector），为每个子 Widget 调用访问者函数                                                                                                                                                       |
| `EventRouter`         | 事件分发：`std::visit` 模式匹配各事件类型 → 分发到设备更新、HitTester::hit_test → Widget::input、主题变更、DPI 调整、调度器、销毁处理。持有 `last_mouse_target_` 弱引用（EventRouter.hpp:59）：MouseMove 命中变化时向旧目标补派同一事件（旧按钮 is_inside=false → hover 清除），再更新并正常派发新目标                                                                                                                                                    |
| `InvalidationTracker` | 脏标记跟踪：`dirty_` 原子标志 + `animation_` 计数器 + 脏 Widget 列表（`shared_mutex` 保护）。提供 `needs_frame()`、`consume_dirty_list()`                                                                                                                                                |
| `MsgPump`             | 线程安全消息队列：有界 SPSC 环形缓冲区（32 槽）+ `std::counting_semaphore<32>` + `std::condition_variable` + `std::jthread`                                                                                                                                                              |
| `RenderScheduler`     | 独立渲染线程：`std::jthread` + `condition_variable`，帧回调驱动。`request_frame()` 唤醒渲染、`set_pending_size()` 处理 resize；动画期间 16ms 节拍出帧（`sleep_for`），`anim_inc` 启动时 `request_frame` 唤醒。`stop()` request_stop + notify 后 join 渲染线程（self-id 守卫，与 MsgPump::stop 对齐） |
| `MutableWidget`       | 变体容器：`std::variant<monostate, list<MutableWidget>, vector<MutableWidget>, shared_ptr<Widget>>`（MutableWidget.hpp:17，子控件 shared_ptr 强持有、树持有所有权），支持四种 Widget 子节点组织方式。提供 `is_null/is_widget/is_list/is_vector` 查询 + `as_widget/as_list/as_vector` 访问器 |

**全局消息流**：

```
WM_* → Platform::translate_event() → MsgPump::push_msg()
  → MsgPump::msg_loop() → EventRouter::dispatch()
    → std::visit → handle_input / handle_resize / handle_dpi_change / handle_theme_change / handle_destroy
      → HitTester::hit_test() → Widget::input() / RenderScheduler::set_pending_size / Context::scheme 更新 / Engine::clear() → TreeManager::clear()
```

**重绘触发**：输入事件本身不触发重绘；仅 Resize/DpiChange/ThemeChanged 或控件内部调用 `mark_dirty()` 时，`dispatch()` 尾部（EventRouter.cpp:70-72）才调用 `request_frame()`。Button hover 进入/离开变化时均调用 `mark_dirty()`（首个真实重绘消费者，点击重绘闭环已通；离开由 EventRouter 向旧目标补派 MouseMove 触发）

### 6. Widget 系统 (`neko::widget`)

- **Widget** 基类：虚方法 `layout(Vec4I, Context&)`、`draw(Vec4I, Context&, Backend&) -> Rect`、`build(Context&)`、`event(Context&)`、`input(Context&, Event)`、`hit_test(Mouse) -> bool`。Protected 成员：`bounds`（`Vec4I`）、`isFocus`（`std::atomic_bool`）、`isDirty`（`std::atomic_bool`）、`context_`。私有成员：`children_`（`MutableWidget`，变体存储 `shared_ptr<Widget>`，见下）、`z_index_`、`id_`、`path_`
  - **Builder API**：`build<T>(Args...) -> T&`（创建子 Widget 并链式配置）、`children(fn)`（lambda 作用域批量创建子节点）、`parent()`（获取父 Widget）
  - **build\<T\> 内部流程**（Widget.hpp:91-125）：`make_shared` 创建子控件 → 设 `parent_` → 插入 `children_` 前取 `std::unique_lock(tree_mutex)`（Widget.hpp:98-102，children_ 突变与渲染线程 layout/draw 遍历互斥，context_/tree_mutex 判空）→ `std::visit` 按当前变体插入 `children_`（monostate → 存为单子；shared_ptr 变体 → 升级为 list；list/vector → `emplace_back`）→ 调用 `context_->widget_tree_changed()`（→ `Engine::schedule_rebuild` 置标志，帧首合并重建）→ 返回 `T&` 供链式配置。**限制**：自定义 Widget 的 `build(Context&)`（在 register_widget 锁内调用）中禁止调用 `build<T>()`（同线程重锁死锁）
  - **已修复**：`children_` 改 `shared_ptr` 强持有，`build<T>` 返回的引用不再悬垂（曾因 `weak_ptr` 无强持有者导致子控件创建后立即销毁）
  - **Style Mixin 继承**：Widget 通过继承 `style::BackgroundStyle`、`style::SizeStyle`、`style::BorderStyle`、`style::TextStyle` 等 mixin 结构体获得样式属性，零运行时开销
- **Button**：继承 `Widget` + `BackgroundStyle`、`SizeStyle`、`BorderStyle`、`TextStyle`，实现 `layout`（SizeStyle 回退 → 固定尺寸/父尺寸）、`draw`（ColorScheme 回退 → 背景/边框/文字居中）、`input`（左键点击 → `on_click` 回调；`MouseMoveEvent` → hover 检测）、`hit_test`（`Mouse::is_inside(bounds)`，原始 bounds 不受缩放影响）。构造函数 `Button(Context&, text="", onClick=nullptr)`，链式 setter `on_click()`/`text()`。私有成员 `hover_` + `component::Animation<float> scale_{1.0F, 200}`（200ms 缩放动画，构造时 `scale_.bind(context.anim_inc, context.anim_dec)`）：hover 变化时 `scale_.to_value(1.06F/1.0F)` + `context.mark_dirty()` 触发重绘；draw 按 `scale_.tick()` 中心缩放矩形，hover 背景用 `context.scheme.secondaryContainer`（否则 `primary`）
- **Column**：继承 `Widget` + `BackgroundStyle`、`SizeStyle`，垂直布局容器。实现 `layout`（垂直栈式布局子 Widget，经 `visit_children` 遍历）和 `draw`（仅背景绘制，不遍历子节点）、`build`/`event`/`input`/`hit_test`
- **Row**：继承 `Widget` + `BackgroundStyle`、`SizeStyle`，水平布局容器。实现 `layout`（水平栈式布局子 Widget）和 `draw`（背景绘制）、`hit_test`
- **Center**：继承 `Widget` + `BackgroundStyle`，居中布局容器。实现 `draw`（ColorScheme 回退 → 背景绘制）、`layout`（计算子 Widget 自然尺寸后居中放置）、`hit_test`

### 7. 样式系统 (`neko::style`)

- **ColorScheme**：Material You 36 色调配色方案。包含 **HCT（Hue/Chroma/Tone）色彩引擎**完整实现（sRGB ↔ XYZ ↔ CAM16 ↔ HCT 转换管线）：
  - sRGB 线性化/反线性化
  - D65 XYZ 矩阵变换
  - CAM16 色表模型（Hunt-Pointer-Estevez 矩阵 + 色适应函数）
  - HCT→sRGB 牛顿迭代求解（5 轮 + 16 轮二分 chroma 回退）
  - `light(seed)` / `dark(seed)` 工厂方法生成 36 色调色板
- **CSS 基础结构**：`Background`（Color）、`Size`（size/margin/padding）、`Border`（size/color）
- **Style Mixin 结构体**：`BackgroundStyle`、`SizeStyle`、`BorderStyle`、`TextStyle` 四个 mixin 结构体，每个包含对应样式成员（`background_`、`size_`、`border_`、`text_color_`、`font_size_`）。Widget 通过多重继承选择所需的 mixin，**零运行时开销**，无字符串查找，无 hashmap

### 线程模型

- **主线程**: Win32 消息循环（`GetMessageW` / `DispatchMessageW`）
- **渲染线程**: `RenderScheduler` 通过 `std::jthread` 驱动帧
- **消息线程**: `MsgPump` 通过 `std::jthread`（有界 SPSC 环形缓冲区 + 信号量）
- **树结构并发保护**（`tree_manager_.mutex_`，`std::shared_mutex`）：`Widget::build<T>` 插入 children_ 前取 `unique_lock`（突变）；`Engine::render_frame` 在 root->layout + draw_widget 外围持 `shared_lock`（Engine.cpp:128）；`HitTester` 遍历同持 shared_lock 读锁（HitTester.cpp:11）——children_ 突变与布局/绘制/命中遍历互斥，消除启动期树竞争 UB

### 设计模式

| 模式       | 使用位置                                                                                  |
| ---------- | ----------------------------------------------------------------------------------------- |
| 策略模式   | `Backend` 抽象基类 + `DirectX11` 具体实现                                                 |
| 模板方法   | `Platform` 基类 + `Win32` 具体实现                                                        |
| 单例模式   | `Platform::instance()`                                                                    |
| 组合模式   | Widget 树形结构与子节点（`MutableWidget` 变体容器）                                       |
| 观察者模式 | `InvalidationTracker` 脏标记 + `AnimationBase` 动画跟踪 + `ValueState` 绑定               |
| 代理模式   | `MsgPump` 作为线程安全事件代理                                                            |
| 工厂方法   | `Engine::set_root_widget<T>()`、`register_platform<T>()`                                  |
| 访问者模式 | `WidgetVisitor::visit_children` 统一分发 `MutableWidget` 四种变体                         |
| CRTP       | `Stylable<Derived>` 为 Widget 提供链式 `.style()` 调用（已移除，替换为 style mixin 继承） |

## Source Tree

```
NekoUI/                                    ← 项目根（.slnx, AGENTS.md, .clang-format, README.md）
└── NekoUI/                                ← VS 项目目录（含 main.cpp, .vcxproj）
    ├── main.cpp                           # 入口：main()、窗口创建、消息循环、Engine 启动、示例 UI
    ├── NekoUI.vcxproj                     # VS 项目文件
    ├── NekoUI.vcxproj.filters             # VS 项目筛选器
    └── NekoUI/                            ← 库核心目录
        ├── NekoUI.hpp                     # 主包含头文件（转发 Engine.hpp）
        ├── Type.hpp                       # 核心类型：Vec2/3/4<T>（union xyzw/rgba）、Color（uint32 RGBA）、Handle（void*）
        ├── Backend/
        │   ├── Backend.hpp                # 渲染后端抽象基类（策略模式）
        │   ├── stb_truetype.h             # 嵌入式字体光栅化（5079 行，gitignored — 不跟踪）
        │   └── DirectX11/
        │       ├── DirectX11.hpp          # D3D11 实现头文件（设备/交换链/着色器/字体图集/CJK 字形/TextCB）
        │       └── DirectX11.cpp          # D3D11 完整实现
        ├── Component/
        │   ├── ValueState.hpp             # 响应式值封装 + 脏标记绑定
        │   └── Animation.hpp              # 12 种速率曲线动画引擎（376 行）
        ├── Device/
        │   ├── Keyboard.hpp               # 键盘状态（修饰键 + 边沿检测 + 字符缓冲）
        │   └── Mouse.hpp                  # 鼠标状态 + DPI 感知 4 种命中测试（153 行）
        ├── Engine/
        │   ├── Context.hpp                # 引擎共享上下文（回调 + Mouse/Keyboard 弱引用 + ColorScheme）
        │   ├── Engine.hpp                 # 引擎主类声明
        │   ├── Engine.cpp                 # 引擎实现：初始化全部子系统并连接回调
        │   ├── EventRouter.hpp            # 事件路由声明
        │   ├── EventRouter.cpp            # std::visit 事件分发
        │   ├── HitTester.hpp              # 命中测试声明（递归反向遍历，返回顶层命中 Widget）
        │   ├── HitTester.cpp              # 命中测试实现
        │   ├── InvalidationTracker.hpp    # 脏标记跟踪声明
        │   ├── InvalidationTracker.cpp    # 实现（atomic dirty + animation 计数器 + 脏 Widget 列表）
        │   ├── MsgPump.hpp                # 线程安全消息队列声明
        │   ├── MsgPump.cpp                # SPSC 环形缓冲区实现（32 槽）
        │   ├── MutableWidget.hpp          # 变体 Widget 容器（四种形式：monostate/list/vector/shared_ptr）
        │   ├── RenderScheduler.hpp        # 独立渲染线程声明
        │   ├── RenderScheduler.cpp        # 渲染线程实现
        │   ├── TreeManager.hpp            # Widget 树管理声明（root/focus 原子指针 + ID/index 映射 + 焦点导航）
        │   ├── TreeManager.cpp            # Widget 树管理实现（shared_mutex 线程安全）
        │   ├── WidgetBuilder.hpp          # Widget Builder API 声明
        │   ├── WidgetBuilder.cpp          # Widget 构建遍历实现（注册到 ID/index 映射，调用 build()）
        │   └── WidgetVisitor.hpp          # Widget 子节点遍历模板（visit_children，统一四种变体分发）
        ├── Platform/
        │   ├── Event.hpp                  # 事件类型（variant：9 种事件 + Overloaded 工具）
        │   ├── Platform.hpp               # 平台基类（单例，含 IME / 11 窗口操作接口）
        │   └── Win32/
        │       ├── Win32.hpp              # Win32 平台实现声明（TSF IME 状态 + 主题缓存）
        │       └── Win32.cpp              # WM_* → Event 翻译 + 注册表主题检测
        ├── Style/
        │   ├── ColorScheme.hpp            # Material You 36 色调色板结构体
        │   ├── ColorScheme.cpp            # HCT 色彩引擎完整实现
        │   ├── CSS.hpp                    # 基础样式结构体（Background/Size/Border）+ Style Mixin 结构体（BackgroundStyle/SizeStyle/BorderStyle/TextStyle）
        └── Widget/
            ├── Widget.hpp                 # Widget 基类声明（含 Builder API：build<T>/children/parent）
            ├── Widget.cpp                 # Widget 默认实现
            ├── Button/
            │   ├── Button.hpp             # Button 声明（继承 Widget + Style Mixins）
            │   └── Button.cpp             # Button 实现：draw/input/hit_test/on_click/text
            └── Layout/
                ├── Center.hpp             # Center 布局声明（继承 Widget + Style Mixins）
                ├── Center.cpp             # Center 实现：背景绘制 + 子 Widget 居中
                ├── Column.hpp             # Column 垂直布局声明（继承 Widget + Style Mixins）
                ├── Column.cpp             # Column 实现：背景绘制
                ├── Row.hpp                # Row 水平布局声明（继承 Widget + Style Mixins）
                └── Row.cpp                # Row 实现：背景绘制
```

## Build

- **构建命令**: 在 Visual Studio 2022+ 中打开 `NekoUI.slnx` 并构建，或通过命令行：
  ```bat
  msbuild NekoUI.slnx
  ```

### 构建配置

| 配置      | 平台    | 工具集                     | 语言标准      | 运行时库     | 备注                                                                                                            |
|---------|-------|-------------------------|-----------|----------|---------------------------------------------------------------------------------------------------------------|
| Debug   | Win32 | v145 (MSVC)             | C++20     | 默认（/MDd） | SDL 检查开启                                                                                                      |
| Release | Win32 | v145 (MSVC)             | C++20     | 默认（/MD）  | 全程序优化                                                                                                         |
| Debug   | x64   | Intel C++ Compiler 2026 | C++latest | /MTd     | TBB/IPP(Static)/MKL(Parallel)/DAL/MPI、ARROWLAKE-S、Async 异常、InterproceduralOptimization、EnableSegmentHeap      |
| Release | x64   | Intel C++ Compiler 2026 | C++latest | /MT      | 同上 + PGO Instrumentation、CFG Guard、MaxSpeedHighLevel、StringPooling、FavorSizeOrSpeed=Speed、GuardEHContMetadata |

- **平台映射**: `.slnx` 中平台名为 `x86`（`<Platform Name="x86" />`），对应 vcxproj 内部配置名 Win32
- **Win32 运行时库**: vcxproj 未显式声明 `RuntimeLibrary`，/MDd、/MD 为 `UseDebugLibraries` 默认值
- **x64 Debug 补充**: `InterproceduralOptimization=true`、`BrowseInformation=false`（避免 BK1506 编译错误）、`LocalDebuggerCommand=$(TargetPath)`（修复无法启动调试）、`EnableSegmentHeap=true`
- **x64 Release 补充**: `StringPooling=true`、`InlineFunctionExpansion=AnySuitable`、`FavorSizeOrSpeed=Speed`、`GuardEHContMetadata=true`、PGO `ProfileDirectory=coverage.profraw`
- **ARROWLAKE-S**: 对应 `GenerateAlternateCodePaths=ARROWLAKE-S` + `UseProcessorExtensions=ARROWLAKE-S`
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

| 依赖                        | 用途                                                            | 版本/来源                          |
| --------------------------- | --------------------------------------------------------------- | ---------------------------------- |
| DirectX 11 (`d3d11.h`)      | GPU 渲染 API                                                    | Windows SDK                        |
| DXGI (`dxgi1_2.h`)          | 交换链管理                                                      | Windows SDK                        |
| DirectXMath                 | 数学运算                                                        | Windows SDK                        |
| Windows SDK                 | Win32 API + 注册表 API                                          | 系统组件                           |
| Intel oneAPI 2026（仅 x64） | TBB（并行）、IPP（图像处理）、MKL（数学）、DAL（数据分析）、MPI | Intel oneAPI 2026                  |
| stb_truetype.h              | 字体光栅化（编译期包含，gitignored — 不跟踪）                   | 项目内嵌（Backend/stb_truetype.h） |
| MSVC v143/v145              | C++ 标准库                                                      | Visual Studio 2022                 |

**无**外部包管理器（无 vcpkg、无 NuGet、无 npm）— 所有依赖均通过 Windows SDK 和 Visual Studio/Intel 工具集解析。

## Current Status

- **草稿状态** — 核心架构已建立，渲染和交互链路可运行
- **已实现**：跨平台渲染后端抽象（D3D11 实现）、HCT Material You 色彩引擎（sRGB→XYZ→CAM16→HCT 完整管线）、平台抽象（IME TSF / 11 窗口操作 / 注册表主题检测 + 强调色）、响应式 Widget 树（含焦点导航）、Widget Builder API（`build<T>()` / `children()`）、编译期 style mixin 继承（零运行时开销）、12 种速率曲线动画引擎、线程安全事件传递、系统主题变化检测与传递（Light/Dark + AccentColor）、全部核心 Widget（Button/Center/Column/Row）已实现绘制和交互、控件树 `shared_ptr` 所有权（子控件强持有）、rebuild 帧首合并（消除 build<T> 自死锁与 O(n²) 重建风暴）、布局 + 前序 DFS 递归绘制驱动、动画接线（anim_inc 唤醒 + 16ms 节拍）、Button hover 视觉态与 200ms 缩放动画、hover 离开清除（EventRouter `last_mouse_target_` 向旧目标补派 MouseMove）、关窗线程安全（RenderScheduler::stop join 渲染线程，self-id 守卫）、树结构并发加锁（build\<T\> unique_lock / render_frame shared_lock）、demo 3 按钮固定尺寸（120×40）纵向布局
- **架构重构**：引擎核心已从单块 `WidgetTree` 拆分为 `TreeManager`（树数据 + ID 映射）、`HitTester`（命中测试）、`WidgetBuilder`（构建遍历）、`WidgetVisitor`（子节点分发）四个独立组件（`Renderer` 已删除，渲染驱动并入 `Engine::render_frame`），贯彻单一职责原则
- **样式系统重构（已完成）**：移除运行时 `StyleSheet` hashmap 样式表和 `Stylable` CRTP，替换为编译期 style mixin 继承（`BackgroundStyle`/`SizeStyle`/`BorderStyle`/`TextStyle`），零运行时开销，无字符串查找。Widget 通过多重继承选择所需 mixin，`class_name_` 和 `style()` 链式调用一并移除
- **布局下放（已完成）**：布局计算曾从 Engine 集中式 Renderer 下放到各 Widget——Widget 基类新增 `layout()` 虚方法，Column/Row/Center 各自实现子节点定位逻辑，Button 实现自身尺寸计算，`horizontal_` 成员从 Widget 基类移除。`Engine::render_frame` 每帧调用 `root->layout({0,0,w,h})`（草稿期每帧全量布局）驱动布局阶段
- **未实现**：无可运行的测试、无裁剪/溢出处理、无 margin/padding 支持、无最小/最大尺寸约束、无 Wrap/基线对齐等高级布局特性
- **断点**：
  - ✅ 已修复：子控件 `weak_ptr` 无强持有者创建即销毁 → `children_` 改 `shared_ptr` 强持有，子控件由树持有所有权
  - ✅ 已修复：无布局阶段驱动 → `render_frame` 每帧调用 `root->layout({0,0,w,h})`，布局阶段已接通
  - ✅ 已修复：draw 不递归、子树不渲染 → `draw_widget` 前序 DFS 递归绘制（引擎集中式）
  - ✅ 已修复：动画引擎未接线 → `anim_inc` 绑定 `request_frame` 唤醒渲染线程（Engine.cpp:30-35），动画期间 16ms 节拍出帧（RenderScheduler.cpp:54-58），Button hover 缩放为首个使用者
  - ✅ 已修复：输入事件不触发重绘 → Button hover 进入/离开变化均调用 `mark_dirty()` 接通重绘闭环（首个真实调用者；hover 离开由 EventRouter 向旧目标补派 MouseMove，离开也还原）
- **主题色获取**：使用注册表 `HKCU\...\Windows\DWM\AccentColor`（ABGR → RGBA 转换），不再使用已过时的 `DwmGetColorizationColor`
