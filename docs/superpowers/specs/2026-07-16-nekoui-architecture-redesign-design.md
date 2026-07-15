# NekoUI 架构重设计

> 日期：2026-07-16
> 状态：设计已确认，进入实现计划阶段
> 范围：保留控件状态（retained mode）+ 即时模式渲染命令（immediate-mode render commands）+ 按需渲染（on-demand rendering）+ 易用 API

## 1. 背景与目标

NekoUI 当前是一个早期阶段的自研 C++/DirectX11 桌面 UI 框架。原始 `Engine` 是一个 God Class（22 项职责：消息泵、渲染调度、控件树、脏标记、事件路由、动画绑定、状态绑定全部耦合在一个类里），且存在循环依赖（`Engine` ↔ `Widget` 互相 include）、空实现虚函数（`MsgLoop`）、死代码（`build()`/`get_widget()`）和若干并发 bug。

重设计目标：

| 目标 | 说明 |
|------|------|
| 保留控件状态 | Widget 持有自己的状态（文本、颜色、子控件树），框架不每帧重建 |
| 即时模式渲染命令 | Widget 在 `draw()` 时向 Backend 提交渲染命令（矩形/文本），Backend 消费并 GPU 绘制 |
| 按需渲染 | 仅在状态变化（脏标记）或动画进行中渲染；空闲时渲染线程阻塞睡眠，不空转 |
| 易用 API | 组合式 + 属性/回调风格（`add<T>()` + `set_text().set_color().set_on_click()`），用户不写继承 |

## 2. 设计原则

- **组合优于继承**：Engine 内部用组合（持有组件），用户 API 也用组合（Widget 持有子 Widget）
- **解耦层（Context）**：Widget / Animation / ValueState 不直接依赖 Engine，只依赖轻量 `Context`（callback + weak ref）
- **脏驱动渲染**：渲染与否由 `InvalidationTracker` 决定，而非固定帧率
- **线程分离**：消息线程（Win32 消息 → 事件路由）与渲染线程（布局 + 绘制 + Present）解耦，通过 `InvalidationTracker` 的脏标记和 `RenderScheduler` 的条件变量协调
- **零 Engine* 依赖**：Widget 构造函数接收 `Context&`，通过 Context 回调触发 `mark_dirty` / `anim_inc` / `reg_widget`

## 3. 架构总览

```
Application
  └── Engine (编排器，final，无继承)
        ├── owns → Backend (DX11 渲染后端)
        ├── owns → Context (解耦层：callback + weak ref)
        ├── owns → Mouse / Keyboard (输入状态机)
        ├── owns → WidgetTree (控件树: root/focused/id_widgets)
        ├── owns → InvalidationTracker (脏标记 + 动画计数器)
        ├── owns → RenderScheduler (渲染调度: render_thread + cv 等待 + resize 状态)
        ├── owns → MsgPump (消息泵: msg_thread + 环形队列 + counting_semaphore)
        └── owns → EventRouter (事件路由: Win32 消息 → Widget 树)
```

### 线程模型

- **消息线程**（`MsgPump`）：Win32 `WndProc` 把消息 `push_msg` 进环形队列 → 消息线程 `msg_loop` 取出 → 回调 `EventRouter::dispatch()`（mouse/keyboard 状态机 + Widget 树路由 + dirty 检查 → `request_frame`）
- **渲染线程**（`RenderScheduler`）：等待 `render_notify_` 条件变量 → 唤醒后回调 `Engine::render_frame()`（layout + draw + Present）→ 若仍 dirty 或动画计数 > 0 则继续，否则阻塞睡眠

### 渲染流程

```
render_frame():
  1. consume_resize() → backend->resize()（如有 pending resize）
  2. backend->begin()
  3. root = widget_tree_.get_root()
  4. root->layout({0,0,width,height})   // 递归布局
  5. root->draw(*context, *backend)     // 递归提交渲染命令
  6. backend->end()                      // Present
  7. invalidation_.clear()               // 清除脏标记
```

### 事件流程

```
WndProc(msg) → engine.push_msg(msg, w, l)
  → MsgPump 环形队列 → msg_thread
  → EventRouter::dispatch(msg, w, l)
      ├── WM_MOUSE*  → handle_mouse()  → mouse 状态机 → root->raw_event()
      ├── WM_KEY*    → handle_keyboard() → keyboard 状态机 → focused->raw_event()
      ├── WM_DPICHANGED → handle_dpi_change() → backend->set_dpi()
      └── WM_SIZE    → handle_resize() → render_scheduler_->set_pending_size()
  → 若事件导致状态变化 → invalidation_.mark_dirty() → render_scheduler_->request_frame()
```

## 4. 核心抽象

### Context（解耦层）

`Engine/Context.hpp` — 轻量 struct，持有：
- `weak_ptr<Widget> root` — 控件树根（弱引用，避免循环）
- `shared_ptr<Mouse>` / `shared_ptr<Keyboard>` — 输入状态机
- callback：`mark_dirty` / `widget_dirty` / `anim_inc` / `anim_dec` / `reg_widget` / `del_widget`

Widget / Animation / ValueState 只依赖 `Context&`，通过 callback 反向通知 Engine 内部状态变化。这是打破 `Engine` ↔ `Widget` 循环依赖的关键。

### Widget（控件基类）

`Widget/Widget.hpp` — 保留状态：
- `engine::Context* context` 成员（构造时注入，非拥有）
- `Constraints` 布局结构体（x/y/width/height）
- `add<T>()` 子控件工厂（返回 `shared_ptr<T>`）
- 虚方法：`layout()` / `draw()` / `raw_event()` / `hit_test()`

### 组件（Engine 内部）

| 组件 | 职责 |
|------|------|
| `InvalidationTracker` | 脏标记 `dirty_` + 动画计数器 `animation_` + `dirty_list_`（shared_mutex）；`mark_dirty` / `mark_widget_dirty` / `anim_inc` / `anim_dec` / `is_dirty` / `needs_frame` / `clear` |
| `WidgetTree` | `root_`（atomic<shared_ptr>） + `focused_`（atomic<weak_ptr>） + `id_widgets_`（map + shared_mutex）；`set_root` / `get_root` / `set_focus` / `get_focus` / `register_widget` / `unregister_widget` / `clear` |
| `RenderScheduler` | `render_thread_`（jthread） + `render_notify_`（cv） + `pending_`（atomic_bool） + resize 状态；`request_frame` / `set_pending_size` / `stop` / `consume_resize` / `pending_size`；`render_loop` / `render_wait` 内部线程函数；`FrameCallback` 回调注入（`Engine::render_frame`） |
| `MsgPump` | `msg_thread_`（jthread） + 环形队列（kQueueSize=32） + `msg_notify_`（cv） + `msg_space_`（counting_semaphore） + `running_`（atomic_bool）；`push_msg` / `stop` / `msg_loop` / `msg_dequeue`；`MessageHandler` 回调注入（`EventRouter::dispatch`） |
| `EventRouter` | Win32 消息 → Widget 树；持有 `WidgetTree` / `Mouse` / `Keyboard` / `Context` / `Backend` / `RenderScheduler` / `InvalidationTracker` 引用；`dispatch()` → `handle_mouse` / `handle_keyboard` / `handle_dpi_change` / `handle_resize` |

## 5. API 设计（方案 B：组合式 + 属性/回调）

用户代码形态：

```cpp
auto ui = engine.set_root_widget<Container>();
auto btn = ui->add<Button>()
    .set_text("Click me")
    .set_color(Color{0xFF3366FF})
    .set_on_click([] { println("clicked"); });
auto label = ui->add<Label>().set_text("Hello NekoUI");
```

- `set_root_widget<T>(args...)` 返回 `shared_ptr<T>`，用户持有根控件
- `add<T>(args...)` 在父控件上创建子控件，返回 `shared_ptr<T>`
- 属性方法返回 `*this&` 支持链式调用（`set_text().set_color()`）
- 回调（`set_on_click` 等）由 Widget 通过 `Context` 注册到事件系统
- Button 等具体控件可仍被继承以自定义 `draw()`，但用户默认不需要继承

## 6. 已完成的重构（Phase 1-5）

| Phase | 内容 | 状态 |
|-------|------|------|
| 1 | 修复编译一致性：Widget 构造函数 + context 成员；MsgLoop 虚函数改纯虚；删除 `build()`/`get_widget()` 死代码 | ✅ |
| 2 | 打破循环依赖：Widget 构造参数 `Engine*` → `Context&`；删除 Widget.cpp/Button.hpp 对 Engine.hpp 的 include；Context 增加 `weak_ptr<Widget> root` | ✅ |
| 3.1 | 提取 `InvalidationTracker`（dirty_/animation_/dirty_list_） | ✅ |
| 3.2 | 提取 `WidgetTree`（root_/focused_/id_widgets_） | ✅ |
| 3.3 | 提取 `RenderScheduler`（render_thread_/cv/resize 状态） | ✅ |
| 3.4 | 提取 `MsgPump`，删除 `MsgLoop` 继承 | ✅ |
| 3.5 | 提取 `EventRouter`（dispatch + handle_*） | ✅ |
| 4 | Engine 瘦身：删除 `bind_animation`/`bind_value_state` 死代码；清理 include/注释/filters | ✅ |
| 5 | 修复 8 个并发/逻辑 bug（render_wait 死锁、msg_dispatch 空指针、TOCTOU、focused 竞态、WM_KEYDOWN fallthrough、Widget engine 成员、MsgLoop 空实现、死代码） | ✅ |

## 7. 后续实现计划（待 writing-plans 细化）

1. **Widget 体系完善**：Container/Layout 控件（垂直/水平/网格布局）、Label、Image、Input 等基础控件
2. **Layout 系统**：`Widget/Layout/` 目录落地，Constraints 计算与递归 layout
3. **Style/CSS 接入**：`Widget/Style/CSS.hpp` 当前仅声明，需接入 Widget 的属性系统
4. **Animation 接入**：Animation/ValueState 已通过 Context 自绑定，需验证端到端动画驱动渲染
5. **渲染命令抽象**：Backend 的 `draw_rect`/`draw_text` 作为 immediate-mode 命令，Widget::draw 提交
6. **输入事件完善**：hit_test 几何（矩形/圆角/圆形/多边形）、focus 管理、键盘导航（Tab 顺序）
7. **示例/测试**：main.cpp 示例覆盖基础交互；考虑引入 gtest 做单元测试

## 8. 关键决策记录

| 决策 | 理由 | 替代方案 |
|------|------|----------|
| 组合式 API（方案 B） | 用户不需写继承；Widget 持有子控件树；Button 仍可继承自定义 paint | 方案 A（纯继承树）/ 方案 C（纯 immediate mode 每帧重建） |
| Context 解耦层 | 打破 Engine↔Widget 循环依赖；Widget 零 Engine* 依赖 | 直接传 Engine*（循环依赖）/ 全局单例（测试困难） |
| 脏驱动按需渲染 | 空闲不空转，省 CPU/GPU；动画时持续渲染 | 固定帧率（浪费资源）/ 纯 immediate mode（每帧重建状态） |
| Engine final 无继承 | 避免 God Class 继承膨胀；组合替代继承 | Engine 继承 MsgLoop（已删除） |

## 9. 已知开放问题（待实现阶段细化）

以下问题在架构层面已确定方向，但具体实现细节留待 `writing-plans` 阶段细化：

### 9.1 跨线程 Widget 属性安全

当前模型：消息线程处理输入 → 可能修改 Widget 属性（如 `set_text` → `reg_widget` → `frame` → `request_frame`）；渲染线程读 Widget 属性（`layout` + `draw`）。

**开放问题**：Widget 的普通属性（text/color/children）不是线程安全的。若消息线程正在修改 `children` 容器，渲染线程同时遍历会崩溃。

**候选方案**：
- A. 单写者假设：所有 Widget 属性修改只在消息线程发生，渲染线程只读；Widget 树结构变更（add/remove）通过 `widget_tree_` 的 `shared_mutex` 保护
- B. Widget 属性加细粒度锁（每个 Widget 一个 mutex）——开销大，且 layout/draw 期间持锁会导致死锁
- C. 渲染线程拷贝快照：每帧开始时深拷贝 Widget 树状态到渲染线程本地——内存开销大

倾向方案 A（单写者 + 树结构锁），需在 `writing-plans` 中明确约束。

### 9.2 渲染命令批处理

Backend 的 `draw_rect` / `draw_text` 当前是即时提交还是批处理？字体图集（stb_truetype 打包 ASCII + CJK 到 4096×4096 纹理）如何管理生命周期？需明确 Backend 的渲染命令接口与批处理策略。

### 9.3 Layout 算法

当前 `Widget::layout()` 是单阶段（直接设置 `Constraints` 的 x/y/width/height）。复杂布局（垂直/水平/网格）是否需要两阶段（measure → arrange）？`Widget/Layout/` 目录当前为空，需明确布局算法。

### 9.4 CSS 映射

`Widget/Style/CSS.hpp` 当前仅声明，未接入。CSS 样式如何映射到 Widget 属性？编译期（模板/constexpr）还是运行期（运行时解析）？需明确接入方式。

### 9.5 Animation → Widget 驱动

`Animation<T>` 已通过 Context 自绑定（`anim_inc`/`anim_dec`），但其 `on_update` 回调如何更新 Widget 的 color/position？是否需要 Widget 暴露"可动画属性"抽象（如 `Animatable<Color>`）？需明确动画与 Widget 属性的绑定机制。
