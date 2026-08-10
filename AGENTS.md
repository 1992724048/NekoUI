# NekoUI

## Overview

NekoUI 是一个 Windows C++ GUI 框架（UI 库），使用 DirectX 11 渲染。处于开发早期（草稿状态）。核心架构包括 DirectX11 渲染后端、响应式 Widget 树、Material You HCT 色彩引擎、速率曲线动画引擎、独立渲染线程和线程安全消息队列。

- **语言标准**: C++20 (Win32) / C++latest (x64)
- **渲染**: DirectX 11
- **平台**: Windows (Win32 API)
- **命名空间**: `neko`，子命名空间包括 `neko::type`、`neko::engine`、`neko::engine::internal`、`neko::widget`、`neko::backend`、`neko::platform`、`neko::device`、`neko::component`、`neko::component::ease`、`neko::style`
- **文档**: 本文档（AGENTS.md）为唯一权威入职文档

## Architecture

分层架构，从底层到上层依次为：

### 1. 平台实现 (`neko::platform`)

平台事件封装与转换。`Win32`（`platform::Win32`）负责将原生消息（如 Win32 `WM_*`）转换为统一 `Event` 变体。**无抽象基类/单例/注册机制**（YAGNI：不计划跨平台，Win32 是唯一实现，由 `main.cpp` 持有 `std::unique_ptr<Win32>` 实例）。

**支持的事件类型**（`Event.hpp`，variant 含 Overloaded 工具）：

- 鼠标/键盘输入（`MouseMoveEvent`、`MouseButtonEvent`、`MouseWheelEvent`、`KeyEvent`、`CharEvent`）
- 窗口尺寸变化（`ResizeEvent`）
- DPI 变化（`DpiChangeEvent`）
- 窗口销毁（`DestroyEvent`，`WM_DESTROY`）
- 系统主题切换（`ThemeChangedEvent`，`WM_SETTINGCHANGE`）

**主题检测细节**（`Win32.cpp`）：

- Light/Dark 模式：读取注册表 `HKCU\Software\Microsoft\Windows\CurrentVersion\Themes\Personalize\AppsUseLightTheme`
- 强调色：读取注册表 `HKCU\Software\Microsoft\Windows\DWM\AccentColor`（ABGR 格式 → 转为 `neko::type::Color` RGBA 格式）
- 主题变更监听：`WM_SETTINGCHANGE` 消息，`lparam == "ImmersiveColorSet"` 时刷新缓存
- `main.cpp` 在 Engine 构造后通过 `MsgPump::push_msg(win32->query_theme())` 推入初始主题

**IME 支持**：`Win32` 提供 IME 输入法控制（TSF 接口，`ITfThreadMgr`、`ITfDocumentMgr`）。

**窗口操作**：11 个方法（show/hide/close/maximize/minimize/restore/destroy/move/resize/focus/set_opacity）。

**消息入口**：`handle_message(msg, wparam, lparam, msg_pump)` 非静态成员方法，由 `main.cpp` 的 WndProc 直接调用。

### 2. 渲染后端 (`neko::backend`)

`DirectX11`（`backend::DirectX11`）是唯一渲染实现（**无抽象基类**——YAGNI：Windows 下只用 D3D11），提供绘制接口：`draw_rect_fill`、`draw_rect`、`draw_line`、`draw_circle_fill`、`draw_text`，以及 `resize`、`set_dpi`、`begin`/`end` 帧管理、`get_dpi_scale`、`get_native_handle`。Rule of Five 显式 delete（不可拷贝/移动）。

包含：

- D3D11 设备/上下文/交换链创建（Debug 下启用 `D3D11_CREATE_DEVICE_DEBUG`）
- HLSL 顶点/像素着色器编译（矩形着色器 + 文本着色器）
- stb_truetype 字体图集生成（含 CJK 字形支持）
- 常量缓冲区管理（`TextCB` 结构体）

### 3. 输入设备 (`neko::device`)

- **Keyboard**：修饰键状态（ctrl/shift/alt）、边沿触发按键检测（`just_pressed`/`just_released`）、字符输入缓冲（16 字符）
- **Mouse**：DPI 感知点击测试——矩形（`is_inside`）、圆形、圆角矩形、多边形（射线法）命中检测；按键边沿检测（`left/right/middle_clicked/released`）；`set_dpi()` 由 `Engine` 构造时按 `get_dpi_scale()*96` 初始化

### 4. 响应式组件 (`neko::component`)

- **ValueState\<T\>**：带脏标记绑定的响应式值封装，赋值时自动触发 `mark_dirty()` 回调
- **Animation\<T, EasingFn, TimeType\>**：支持 **12 种速率曲线**的动画引擎（linear、sine、quad、cubic、quart、quint、expo、circ、back、elastic、bounce），每种曲线含 `in`/`out`/`in_out` 三种缓动方向。`AnimationBase` 管理生命周期（`is_active()`、`bind(on_start, on_end)`）。典型用法见 Button：构造时 `scale_.bind(context.anim_inc, context.anim_dec)` 接线，`to_value(target)` 启动动画

### 5. 引擎核心 (`neko::engine`)

**核心类**：

| 类 | 职责 |
| --- | --- |
| `Engine` | 总控制器：以 `std::shared_ptr` 拥有全部子系统（DirectX11 构造注入、TreeManager、WidgetBuilder、HitTester、MsgPump、RenderScheduler、EventRouter、InvalidationTracker、输入设备、Context），观察者（EventRouter/HitTester/WidgetBuilder/RenderScheduler/MsgPump/Context/Widget）一律持 `weak_ptr`（无 shared_ptr 环；lock 失败安全降级是最后的运行时安全网）。构造函数用 lambda 将全部子系统回调连接。提供 `set_root_widget<T>()` 模板工厂、`render_frame()` 帧绘制、`rebuild()`（外部保留入口）、`schedule_rebuild()` 延迟重建（`tree_dirty_` 标志 + `request_frame`，帧首合并）、`clear()` 关窗清理、私有静态 `draw_widget()` 集中式前序 DFS 递归绘制 |
| `Context` | 引擎共享上下文：`mark_dirty`/`widget_dirty`/`anim_inc`/`anim_dec`/`widget_tree_changed` 回调、Mouse/Keyboard 弱引用、`tree_manager`（`weak_ptr<TreeManager>`，`enable_shared_from_this` 基类，使用时 lock 取 `->mutex_`）、`root`（弱引用根 Widget）、`ColorScheme`、`native_handle` |
| `TreeManager` | Widget 树管理：root/focus 原子指针（`std::atomic<shared_ptr<Widget>>`）、ID→Widget 映射、index→Widget 映射、`next_focus`/`prev_focus` 焦点导航。`register_widget()` 分配 z_index 和路径并调用 `build()`，同时以 `context.shared_from_this()` 填充 `Widget::context_`。`std::shared_mutex` 线程安全 |
| `HitTester` | 命中测试器：递归反向遍历子节点（z 顺序），返回 `std::optional<std::shared_ptr<widget::Widget>>`（顶层命中的 Widget），持有 `std::weak_ptr<TreeManager>`（遍历前 lock，失败返回 nullopt），遍历持 `shared_lock` 读锁 |
| `WidgetBuilder` | 构建遍历器：递归注册 Widget 到 TreeManager 的 ID/index 映射，调用每个 Widget 的 `build()`。持有 `std::weak_ptr<TreeManager>`（build 前 lock，失败 return）。由 `Engine::rebuild()`/`render_frame` 帧首触发 |
| `WidgetVisitor` | 工具模板：`visit_children()` 统一分发 `MutableWidget` 四种变体（monostate/shared_ptr/list/vector），为每个子 Widget 调用访问者函数 |
| `EventRouter` | 事件分发：`std::visit` 模式匹配各事件类型 → 分发到设备更新、`HitTester::hit_test` → `Widget::input`、主题变更、DPI 调整、调度器、销毁处理（绑定 `Engine::clear`）。7 个依赖成员全部为 `std::weak_ptr`（tree/hit_tester/mouse/keyboard/context/backend/invalidation），各 handler 内 lock + 判空（失败降级：丢弃该事件不崩溃）。持有 `last_mouse_target_` 弱引用：MouseMove 命中变化时向旧目标补派同一事件（旧按钮 `is_inside=false` → hover 清除），再更新并正常派发新目标 |
| `InvalidationTracker` | 脏标记跟踪：`dirty_` 原子标志 + `animation_` 计数器 + 脏 Widget 列表（`shared_mutex` 保护）。提供 `needs_frame()`、`consume_dirty_list()`、`mark_dirty()`/`mark_widget_dirty()`、`anim_inc()`/`anim_dec()` |
| `MsgPump` | 线程安全消息队列：有界 SPSC 环形缓冲区（32 槽）+ `std::counting_semaphore<32>` + `std::condition_variable` + `std::jthread`。`stop()` 停止并 join 消息线程 |
| `RenderScheduler` | 独立渲染线程：`std::jthread` + `condition_variable`，帧回调驱动。`request_frame()` 唤醒渲染、`set_pending_size()`/`consume_resize()` 处理 resize；动画期间 16ms 节拍出帧（`sleep_for`），`anim_inc` 启动时 `request_frame` 唤醒。`stop()` request_stop + notify 后 join 渲染线程（self-id 守卫，与 MsgPump::stop 对齐） |
| `MutableWidget` | 变体容器：`std::variant<monostate, list<MutableWidget>, vector<MutableWidget>, shared_ptr<Widget>>`，支持四种 Widget 子节点组织方式。子控件 `shared_ptr` 强持有、树持有所有权。提供 `is_null/is_widget/is_list/is_vector` 查询 + `as_widget/as_list/as_vector` 访问器 |

**全局消息流**：

```
WM_* → Win32::handle_message() → translate_event() → MsgPump::push_msg()
  → MsgPump::msg_loop() → EventRouter::dispatch()
    → std::visit → handle_input / handle_resize / handle_dpi_change / handle_theme_change / handle_destroy
      → HitTester::hit_test() → Widget::input() / RenderScheduler::set_pending_size / Context::scheme 更新 / Engine::clear() → TreeManager::clear()
```

**渲染管线**（`Engine::render_frame`）：`consume_resize` → 帧首合并重建（`tree_dirty_` 交换置位）→ 尺寸兜底（`pending_size` 或 `GetClientRect`）→ `root->layout({0,0,w,h})` → `backend->begin()` → `draw_widget` 前序 DFS 递归绘制 → `backend->end()` → `invalidation_.clear()`

**重绘触发**：输入事件本身不触发重绘；仅 Resize/DpiChange/ThemeChanged 或控件内部调用 `mark_dirty()` 时，`dispatch()` 尾部才调用 `request_frame()`。Button hover 进入/离开变化时均调用 `mark_dirty()`（首个真实重绘消费者，点击重绘闭环已通；离开由 EventRouter 向旧目标补派 MouseMove 触发）

### 6. Widget 系统 (`neko::widget`)

- **Widget** 基类：虚方法 `layout(Vec4I, Context&)`、`draw(Vec4I, Context&, backend::DirectX11&) -> Rect`、`input(Context&, Event)`、`hit_test(Mouse) -> bool`（build/event 已删除），默认实现遍历 `behaviors_` 容器按类型委托给行为组件（无对应行为时空操作/`draw` 返回 `{}`/`hit_test` 返回 false）。Public：`add_behavior<T>(Args&&...) -> T&`（make_unique 存入容器并返回引用，自动注入 `*this` 为首参）、`get_hovered()/set_hovered()`（`std::atomic_bool hovered_`，relaxed 内存序）、`get_bounds()/set_bounds()`。Protected 成员：`bounds`（`Vec4I`）、`isFocus`（`std::atomic_bool`）、`isDirty`（`std::atomic_bool`）、`context_`（`std::weak_ptr<Context>`，所有使用点 lock，失败降级）、`behaviors_`（`std::vector<std::unique_ptr<Behavior>>`）、`hovered_`。私有成员：`children_`（`MutableWidget`，变体存储 `shared_ptr<Widget>`）、`z_index_`、`id_`、`path_`
  - **行为接口层**（`Widget/Behavior/`）：`Behavior` 基类（持 `Widget& owner_` 访问控件数据，Rule of Five delete）+ 4 纯虚接口——`LayoutBehavior`（layout）、`DrawBehavior`（draw -> Rect）、`InputBehavior`（input）、`HitTestBehavior`（hit_test const -> bool）。具体行为实现放各控件目录（如 `Widget/Button/`）
  - **Builder API**：`build<T>(Args...) -> T&`（创建子 Widget 并链式配置）、`children(fn)`（lambda 作用域批量创建子节点）、`parent()`（获取父 Widget）
  - **build\<T\> 内部流程**（`Widget.hpp`）：`context_.lock()` 取 Context → `make_shared` 创建子控件（lock 失败时用临时 shared_ptr Context 构造孤儿控件，仅拷贝回调不依赖 Context 存活）→ 设 `parent_` → 插入 `children_` 前经 `context->tree_manager.lock()` 取 `std::unique_lock(tree->mutex_)`（children_ 突变与渲染线程 layout/draw 遍历互斥，lock 失败跳过加锁）→ `std::visit` 按当前变体插入 `children_`（monostate → 存为单子；shared_ptr 变体 → 升级为 list；list/vector → `emplace_back`）→ 调用 `context->widget_tree_changed()`（→ `Engine::schedule_rebuild` 置标志，帧首合并重建）→ 返回 `T&` 供链式配置。
  - `children_` 为 `shared_ptr` 强持有，`build<T>` 返回的引用不悬垂（曾因 `weak_ptr` 无强持有者导致子控件创建后立即销毁）
  - **组合样式表**：每控件持有一个组合样式结构体（`style::ButtonStyle`/`ColumnStyle`/`RowStyle`/`CenterStyle`），经 `style()` 访问器暴露；行为组件构造时注入样式引用（`add_behavior<ButtonDraw>(style_, ...)`）直接访问，零运行时开销、无 static_cast、无字符串查找
- **Button**：继承仅 `Widget`（样式改持 `style::ButtonStyle style_` 成员 + `style()` 访问器）。**行为组装零 override**：构造时 `add_behavior` 组装——`ButtonLayout`（`style_.size.value` 回退 → 固定尺寸/父尺寸）、`ButtonDraw`（`style_.background.color` 回退 → 背景 hover 时 `scheme.secondary_container` 否则 `scheme.primary`/边框/文字居中；持 `scale_` 缩放动画，draw 内 hovered_ 边沿检测驱动 `to_value(1.06F/1.0F)` + tick）、`ButtonInput`（hover 检测写 `set_hovered` + `mark_dirty`，左键按下且命中 → 回调；不触碰动画）、`ButtonHitTest`（`Mouse::is_inside`）。构造函数 `Button(const Context&, text="", onClick=nullptr)`，链式 setter `on_click()`（经 `ButtonInput& input_` 引用委托）。动画 `component::Animation<float> scale_{1.0F, 200}`（构造时 `scale_.bind(context.anim_inc, context.anim_dec)`）
- **Column**：继承仅 `Widget`（`style::ColumnStyle style_`）。**行为组装零 override**：`ColumnLayout`（垂直栈式布局子 Widget，经 `visit_children` 遍历）、`ColumnDraw`（仅背景绘制，不遍历子节点）、`ColumnHitTest`
- **Row**：继承仅 `Widget`（`style::RowStyle style_`）。**行为组装零 override**：`RowLayout`（水平栈式布局子 Widget）、`RowDraw`（背景绘制）、`RowHitTest`
- **Center**：继承仅 `Widget`（`style::CenterStyle style_`）。**行为组装零 override**：`CenterLayout`（计算子 Widget 自然尺寸后居中放置）、`CenterDraw`（ColorScheme 回退 → 背景绘制）、`CenterHitTest`

### 7. 样式系统 (`neko::style`)

- **ColorScheme**：Material You 色调配色方案。包含 **HCT（Hue/Chroma/Tone）色彩引擎**完整实现（`ColorScheme.cpp`，内部匿名命名空间，约 400 行）：
  - sRGB 线性化/反线性化（`linearized`/`delinearized`）
  - D65 XYZ 矩阵变换（`xyz_from_rgb`）
  - CAM16 色表模型（`ViewingConditions` 缓存单例 + `chromatic_adaptation` 色适应函数 + `cam16_from_xyz`）
  - HCT→sRGB 求解（`find_linear_rgb` 5 轮 J 牛顿迭代 + `bisect_to_limit` 临界平面色相二分回退，对齐 material_color_utilities 0.13.0）
  - 6 组色调色板（`make_palettes`）：primary(36)、secondary(16)、tertiary(+60° hue, 24)、neutral(6)、neutral_variant(8)、error(25° hue, 84) —— 均取 seed 的 hue，tertiary 移位
  - `light(seed)` / `dark(seed)` 静态工厂按 Material You tone 规范生成 47 个色调字段（与 Flutter ColorScheme 的 47 个非废弃角色一一对应）
  - 字段命名注意：`primary`/`secondary`/`surface` 等为 snake_case（`primary_container`、`secondary_container`、`on_primary_container`），仅 `onPrimary`/`onSecondary`/`onTertiary` 为 camelCase（对齐 Material 官方命名）——**引用时以 `scheme.secondary_container` 这类 snake_case 为准**
- **CSS 基础结构**：`Background`（color）、`Size`（value，哨兵 = 用父尺寸）、`Border`（width/color）、`Text`（color/font_size）
- **组合样式表**：`ButtonStyle`/`ColumnStyle`/`RowStyle`/`CenterStyle` 四个聚合结构体（组合基础结构），控件持 `style_` 成员 + `style()` 访问器，行为构造注入样式引用——**零运行时开销**，无字符串查找，无 hashmap

### 线程模型

- **主线程**: Win32 消息循环（`GetMessageW` / `DispatchMessageW`）
- **渲染线程**: `RenderScheduler` 通过 `std::jthread` 驱动帧
- **消息线程**: `MsgPump` 通过 `std::jthread`（有界 SPSC 环形缓冲区 + 信号量）
- **树结构并发保护**（`TreeManager::mutex_`，`std::shared_mutex`）：`Widget::build<T>` 插入 children_ 前取 `unique_lock`（突变）；`Engine::render_frame` 在 root->layout + draw_widget 外围持 `shared_lock`；`HitTester` 遍历同持 `shared_lock` 读锁——children_ 突变与布局/绘制/命中遍历互斥，消除启动期树竞争 UB

### 设计模式

| 模式 | 使用位置 |
| --- | --- |
| 单例模式 | `viewing_conditions()` 缓存单例（HCT 引擎） |
| 组合模式 | Widget 树形结构与子节点（`MutableWidget` 变体容器）+ 控件行为组件组装（`add_behavior<T>()`） |
| 观察者模式 | `InvalidationTracker` 脏标记 + `AnimationBase` 动画跟踪 + `ValueState` 绑定 |
| 代理模式 | `MsgPump` 作为线程安全事件代理 |
| 工厂方法 | `Engine::set_root_widget<T>()`、`ColorScheme::light/dark` |
| 访问者模式 | `WidgetVisitor::visit_children` 统一分发 `MutableWidget` 四种变体 |

## Source Tree

```
NekoUI/                                    ← 项目根（.slnx, AGENTS.md, .clang-format, README.md）
├── tools/
│   └── color-picker/
│       ├── index.html                     # HCT 颜色选取/调试工具入口（零依赖，算法与 ColorScheme.cpp 同源）
│       ├── style.css                      # 工具样式（直角扁平化，主题 CSS 变量）
│       └── app.js                         # 工具逻辑（NekoHCT 引擎 + 页面交互）
└── NekoUI/                                ← VS 项目目录（含 main.cpp, .vcxproj）
    ├── main.cpp                           # 入口：main()、窗口创建、消息循环、Engine 启动、示例 UI
    ├── NekoUI.vcxproj                     # VS 项目文件
    ├── NekoUI.vcxproj.filters             # VS 项目筛选器
    └── NekoUI/                            ← 库核心目录
        ├── NekoUI.hpp                     # 主包含头文件（转发 Engine.hpp）
        ├── Type.hpp                       # 核心类型：Vec2/3/4<T>（union xyzw/rgba）、Color（uint32 RGBA）、Handle（void*）
        ├── Backend/
        │   ├── stb_truetype.h             # 嵌入式字体光栅化（gitignored — 不跟踪，删除后无法从 git 恢复）
        │   └── DirectX11/
        │       ├── DirectX11.hpp          # D3D11 唯一渲染实现头文件（设备/交换链/着色器/字体图集/CJK 字形/TextCB）
        │       └── DirectX11.cpp          # D3D11 完整实现
        ├── Component/
        │   ├── ValueState.hpp             # 响应式值封装 + 脏标记绑定
        │   └── Animation.hpp              # 12 种速率曲线动画引擎
        ├── Device/
        │   ├── Keyboard.hpp               # 键盘状态（修饰键 + 边沿检测 + 字符缓冲）
        │   └── Mouse.hpp                  # 鼠标状态 + DPI 感知 4 种命中测试
        ├── Engine/
        │   ├── Context.hpp                # 引擎共享上下文（回调 + Mouse/Keyboard 弱引用 + ColorScheme）
        │   ├── Engine.hpp                 # 引擎主类声明
        │   ├── Engine.cpp                 # 引擎实现：初始化全部子系统并连接回调 + render_frame + draw_widget
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
        │   └── Win32/
        │       ├── Win32.hpp              # Win32 平台实现声明（TSF IME 状态 + 主题缓存）
        │       └── Win32.cpp              # WM_* → Event 翻译 + 注册表主题检测
        ├── Style/
        │   ├── ColorScheme.hpp            # Material You 47 色调字段结构体（Brightness 枚举 + light/dark 工厂）
        │   ├── ColorScheme.cpp            # HCT 色彩引擎完整实现（约 400 行）
        │   ├── CSS.hpp                    # 基础样式结构体（Background/Size/Border/Text）+ 组合样式表（ButtonStyle/ColumnStyle/RowStyle/CenterStyle）
        └── Widget/
            ├── Widget.hpp                 # Widget 基类声明（含 Builder API：build<T>/children/parent + 行为委托）
            ├── Widget.cpp                 # Widget 默认实现
            ├── Behavior/
            │   ├── Behavior.hpp           # 行为基类（持 Widget& owner_，Rule of Five delete）
            │   ├── LayoutBehavior.hpp     # 布局行为接口（纯虚 layout）
            │   ├── DrawBehavior.hpp       # 绘制行为接口（纯虚 draw -> Rect）
            │   ├── InputBehavior.hpp      # 输入行为接口（纯虚 input）
            │   └── HitTestBehavior.hpp    # 命中测试行为接口（纯虚 hit_test const -> bool）
            ├── Button/
            │   ├── Button.hpp             # Button 声明（继承仅 Widget，style_ 成员 + style() 访问器，行为组装零 override）
            │   ├── ButtonLayout.hpp/.cpp  # 布局行为：style_.size.value 回退尺寸计算
            │   ├── ButtonDraw.hpp/.cpp    # 绘制行为：背景/边框/文字 + hover 边沿检测 + scale_ 动画
            │   ├── ButtonInput.hpp/.cpp   # 输入行为：hover 状态更新 + 点击回调
            │   └── ButtonHitTest.hpp/.cpp # 命中行为：bounds 判定
            └── Layout/
                ├── Center.hpp             # Center 声明（行为组装零 override）
                ├── CenterLayout.hpp/.cpp  # 布局行为：子自然尺寸 + 居中放置
                ├── CenterDraw.hpp/.cpp    # 绘制行为：背景绘制（ColorScheme 回退）
                ├── CenterHitTest.hpp/.cpp # 命中行为
                ├── Column.hpp             # Column 声明（行为组装零 override）
                ├── ColumnLayout.hpp/.cpp  # 布局行为：垂直栈式布局
                ├── ColumnDraw.hpp/.cpp    # 绘制行为：背景绘制
                ├── ColumnHitTest.hpp/.cpp # 命中行为
                ├── Row.hpp                # Row 声明（行为组装零 override）
                ├── RowLayout.hpp/.cpp     # 布局行为：水平栈式布局
                ├── RowDraw.hpp/.cpp       # 绘制行为：背景绘制
                └── RowHitTest.hpp/.cpp    # 命中行为
```

## Build

- **构建命令**: 在 Visual Studio 2022+ 中打开 `NekoUI.slnx` 并构建，或通过命令行：
  ```bat
  msbuild NekoUI.slnx
  ```

### 构建配置

| 配置 | 平台 | 工具集 | 语言标准 | 运行时库 | 备注 |
| --- | --- | --- | --- | --- | --- |
| Debug | Win32 | v145 (MSVC) | C++20 | 默认（/MDd） | SDL 检查开启 |
| Release | Win32 | v145 (MSVC) | C++20 | 默认（/MD） | 全程序优化 |
| Debug | x64 | Intel C++ Compiler 2026 | C++latest | /MTd | TBB/IPP(Static)/MKL(Parallel)/DAL/MPI、ARROWLAKE-S、Async 异常、InterproceduralOptimization、EnableSegmentHeap |
| Release | x64 | Intel C++ Compiler 2026 | C++latest | /MT | 同上 + PGO Instrumentation、CFG Guard、MaxSpeedHighLevel、StringPooling、FavorSizeOrSpeed=Speed、GuardEHContMetadata |

- **平台映射**: `.slnx` 中平台名为 `x64` / `x86`（`<Platform Name="x86" />`），对应 vcxproj 内部配置名 Win32
- **Win32 运行时库**: vcxproj 未显式声明 `RuntimeLibrary`，/MDd、/MD 为 `UseDebugLibraries` 默认值
- **调试器设置**: Debug 配置（x64 + Win32）均显式声明 `LocalDebuggerCommand=$(TargetPath)` + `DebuggerFlavor=WindowsLocalDebugger`（防止 Intel 工具链默认调试器覆盖导致"无法启动调试"）
- **x64 Debug 补充**: `InterproceduralOptimization=true`、`BrowseInformation=false`（避免缺失 .sbr 文件导致 BK1506 编译错误）、`EnableSegmentHeap=true`、`RuntimeTypeInfo=true`
- **x64 Release 补充**: `StringPooling=true`、`InlineFunctionExpansion=AnySuitable`、`FavorSizeOrSpeed=Speed`、`GuardEHContMetadata=true`、PGO `ProfileDirectory=coverage.profraw`
- **ARROWLAKE-S**: 对应 `GenerateAlternateCodePaths=ARROWLAKE-S` + `UseProcessorExtensions=ARROWLAKE-S`
- **输出目录**: `$(SolutionDir)$(Platform)-$(Configuration)/`（如 `x64-Debug/`）
- **中间目录**: `$(SolutionDir)$(Platform)-$(Configuration)/.tmep/`（拼写即如此）
- **子系统**: 控制台（Console）
- **DPI 感知**: PerMonitorHighDPIAware (x64)
- **注意**: x64 配置使用 Intel C++ Compiler 2026，需要在开发机上安装 Intel oneAPI 2026 工具集；主题色改走注册表读取，不再依赖已移除的 `roapi.lib`

## Test

当前无测试框架或测试目录。功能验证依赖 `main.cpp` 的 demo UI 人工运行。

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
| --- | --- | --- |
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
- **现代 C++**: 使用 C++20/C++latest 特性，如 `std::optional`、`std::span`、`std::jthread`、`std::shared_mutex`、`std::atomic`、`std::counting_semaphore`、`constexpr`、`[[nodiscard]]`、设计初始化器（`.x = value`）、结构化绑定
- **智能指针**: 优先 `std::unique_ptr` / `std::shared_ptr`，避免裸指针所有权；跨线程共享用 `shared_ptr` + 弱引用（`Context::mouse` 等）
- **标准数组**: 优先 `std::array<T, N>` 替代 C 样式数组 `T[N]`
- **数学常量**: 优先使用 `std::numbers` 中的常量（如 `std::numbers::pi_v<float>`），禁止重复定义标准数学常量
- **数值边界**: 使用 `std::numeric_limits<T>` 获取数值上下限（如 Button layout 用 `numeric_limits<float>::max()` 判父尺寸）
- **删除拷贝语义**: 不可拷贝的类显式 `= delete` 拷贝构造/赋值
- **异常处理**: 使用异常（`try`/`catch`），Async 异常处理模型
- **Lambda**: 使用现代 C++ lambda，显式 trailing return type（`-> void`/`-> Rect` 等），优先值捕获或显式引用捕获
- **`std::bind` 慎用**: 仅用于一次性回调连接（Engine 构造），可读性差时优先 lambda

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
- Include 路径使用相对路径（`../Type.hpp`、`../../Backend/DirectX11/DirectX11.hpp`）

## Dependencies

| 依赖 | 用途 | 版本/来源 |
| --- | --- | --- |
| DirectX 11 (`d3d11.h`) | GPU 渲染 API | Windows SDK |
| DXGI (`dxgi1_2.h`) | 交换链管理 | Windows SDK |
| DirectXMath | 数学运算 | Windows SDK |
| Windows SDK | Win32 API + 注册表 API | 系统组件 |
| Intel oneAPI 2026（仅 x64） | TBB（并行）、IPP（图像处理）、MKL（数学）、DAL（数据分析）、MPI | Intel oneAPI 2026 |
| stb_truetype.h | 字体光栅化（编译期包含，**gitignored — 不跟踪**） | 项目内嵌（Backend/stb_truetype.h） |
| MSVC v143/v145 | C++ 标准库 | Visual Studio 2022 |

**无**外部包管理器（无 vcpkg、无 NuGet、无 npm）— 所有依赖均通过 Windows SDK 和 Visual Studio/Intel 工具集解析。

## Current Status

- **草稿状态** — 核心架构已建立，渲染和交互链路可运行
- **已实现**：DirectX11 渲染后端、HCT Material You 色彩引擎（sRGB→XYZ→CAM16→HCT 完整管线 + 6 组色调色板）、Win32 平台实现（IME TSF / 11 窗口操作 / 注册表主题检测 + 强调色）、响应式 Widget 树（含焦点导航）、Widget Builder API（`build<T>()` / `children()`）、组合样式表（零运行时开销）、12 种速率曲线动画引擎、线程安全事件传递、系统主题变化检测与传递（Light/Dark + AccentColor）、全部核心 Widget（Button/Center/Column/Row）已实现绘制和交互、控件树 `shared_ptr` 所有权（子控件强持有）、rebuild 帧首合并（消除 build<T> 自死锁与 O(n²) 重建风暴）、布局 + 前序 DFS 递归绘制驱动、动画接线（anim_inc 唤醒 + 16ms 节拍）、Button hover 视觉态与 200ms 缩放动画、hover 离开清除（EventRouter `last_mouse_target_` 向旧目标补派 MouseMove）、关窗线程安全（RenderScheduler::stop join 渲染线程，self-id 守卫）、树结构并发加锁（build\<T\> unique_lock / render_frame shared_lock）、HCT 数学偏差修正、Button 构造参数 const 化与 draw_widget 静态化
- **架构重构**：引擎核心已从单块 `WidgetTree` 拆分为 `TreeManager`（树数据 + ID 映射）、`HitTester`（命中测试）、`WidgetBuilder`（构建遍历）、`WidgetVisitor`（子节点分发）四个独立组件（`Renderer` 已删除，渲染驱动并入 `Engine::render_frame`），贯彻单一职责原则
- **样式组合化（已完成）**：移除 style mixin 继承（`BackgroundStyle`/`SizeStyle`/`BorderStyle`/`TextStyle`），替换为组合样式表——基础结构体重构（`Size` 删 margin/padding 死字段、`size`→`value`、`Border::size`→`width`、新增 `Text`）+ 每控件聚合样式结构体（`ButtonStyle`/`ColumnStyle`/`RowStyle`/`CenterStyle`），控件持 `style_` 成员 + `style()` 访问器，行为构造注入样式引用（`const style::XxxStyle&`）——消灭 static_cast 样式访问，零运行时开销
- **布局下放（已完成）**：布局计算曾从 Engine 集中式 Renderer 下放到各 Widget——Widget 基类新增 `layout()` 虚方法，Column/Row/Center 各自实现子节点定位逻辑，Button 实现自身尺寸计算，`horizontal_` 成员从 Widget 基类移除。`Engine::render_frame` 每帧调用 `root->layout({0,0,w,h})`（草稿期每帧全量布局）驱动布局阶段
- **行为组件化（已完成）**：Widget 行为拆分——Task 1 行为接口层（`Widget/Behavior/`：Behavior 基类 + Layout/Draw/Input/HitTest 四接口，持 `Widget& owner_`）；Task 2 Widget 基类改造（4 虚方法默认实现遍历 `behaviors_` 容器按类型委托、`add_behavior<T>()` 组装模板、`hovered_` 提升为 atomic 交互状态修复跨线程病灶、删除 build/event 空壳及全部调用点）；Task 3/4 全部控件重构为行为组装零 override（Button 四行为 + Column/Row/Center 各三行为），全部旧 cpp 删除、vcxproj/filters 同步。**新控件开发 = 组合行为 + 数据，不再 override 虚方法**（样式经组合样式表注入，见"样式组合化"）
- **抽象层砍除（已完成）**：按 YAGNI 移除 `neko::platform` 平台抽象层（基类/单例/工厂注册宏）与 `neko::backend` 渲染抽象层（策略基类），`Win32` 与 `DirectX11` 成为直接实现（无基类、无 override），`handle_message` 改非静态成员方法；初始主题推送从 Engine 构造移至 main.cpp（`win32->query_theme()` 经 MsgPump push）；`Widget::draw` 签名改 `backend::DirectX11&`（Widget.hpp 前置声明避免 D3D11 头渗透）；净减 321 行；顺带修复 Engine 构造 use-after-move 潜在 bug 与 DirectX11 Rule of Five
- **所有权模型改造（已完成）**：Engine 以 `shared_ptr` 拥有全部子系统（context/backend/invalidation/tree_manager/widget_builder/hit_tester/render_scheduler/msg_pump/event_router），所有观察者改持 `weak_ptr`（EventRouter 7 个依赖、HitTester/WidgetBuilder 的 tree、RenderScheduler 的 invalidation、Context 的 tree_manager、Widget 的 context_、MsgPump handler 捕获的 router）——无 shared_ptr 环；各使用点 lock + 判空（lock 失败安全降级：丢弃事件/返回 nullopt/构造孤儿控件，不 UB）。`Context` 增加 `enable_shared_from_this` 基类供 TreeManager 填充 `Widget::context_`；`Widget::build<T>` 的树锁改经 `context->tree_manager.lock()->mutex_` 获取；构造回调统一改 lambda（`std::bind` 清除）
- **生命周期契约修复（已完成）**：`Engine::clear()` 不再提前 reset 被 EventRouter 引用的 backend/context/mouse/keyboard（此前在消息线程内销毁被引用对象，留下悬垂窗口），资源释放推迟至 ~Engine 成员析构（RAII 正统）；Engine.hpp 成员区补生命周期契约注释（Engine 拥有全部子系统 shared_ptr，观察者 weak_ptr；成员声明顺序仍保证观察者先析构，weak_ptr 过期只是最后的运行时安全网）
- **工具（已完成）**：`tools/color-picker/`（index.html + style.css + app.js，零依赖 file:// 即用）HCT 颜色选取/调试工具（直角扁平化 UI，两栏布局：左栏 Seed 颜色+导出 C++、右栏 47 角色表）——seed 输入（取色器 + hex 文本框 + 随机按钮 + 常用颜色列表）+ 6 组色调色带（tone 5..95）+ 47 角色 ColorScheme 表（色块展示，名称/值居中，hover 富提示框显示五色值 RGB/HSL/HSV/CMYK/VEC4）+ 手动 Light/Dark 主题切换（分段滑块动画，初始跟随系统，页面主题色由 scheme() 角色派生）+ 自定义右键菜单（选色器入口/复制颜色格式子菜单）+ 控件配色参考覆盖层 + 导出 ColorScheme 角色字典（名称 → 0xAARRGGBB 两段）；JS 引擎与 ColorScheme.cpp 同源（MCU 0.13.0 对齐）
- **未实现**：无可运行的测试、无裁剪/溢出处理、无 margin/padding 支持、无最小/最大尺寸约束、无 Wrap/基线对齐等高级布局特性
- **主题色获取**：使用注册表 `HKCU\...\Windows\DWM\AccentColor`（ABGR → RGBA 转换），不再使用已过时的 `DwmGetColorizationColor`
