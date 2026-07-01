# 保留模式控件系统 — 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 NekoUI 从单 Widget 即时模式重构为保留模式控件系统，支持多根控件、自动子控件遍历、Z-Order 事件路由和布局容器。

**Architecture:** Widget 基类管理子控件列表（`register_child` 私有，通过 `Sub<T>` 友元类调用）。引擎持有 `vector<unique_ptr<Widget>>` 根控件列表，遍历时自动递归子控件。`Sub<T>` 在构造时自动注册到父控件。

**Tech Stack:** C++26, DirectX 11, GLM, MSBuild

## Global Constraints

- 尾置返回类型: `auto foo() -> void`
- 命名空间: `neko::widget`, `neko::engine`, `neko::backend`, `neko::animation`
- Keep existing `Widget` lifecycle: `draw` / `layout` / `update` / `handle_event`
- Widget `m_bounds`: `glm::ivec4(x, y, width, height)` — 不是 x1/y1/x2/y2
- `type::Color` = `glm::ivec4` (RGBA, 0-255)
- C++26, MSVC v145 (Win32) / Intel C++ 2026 (x64)
- Sub<T> 头部: `Widget/Sub.hpp`

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `NekoUI/NekoUI/Widget/Widget.hpp` | 修改 | Widget 基类：添加树结构、bounds、z-order、dirty 传播、默认遍历实现 |
| `NekoUI/NekoUI/Widget/Sub.hpp` | 新建 | `Sub<T>` 包装器：构造时自动注册，析构时自动注销 |
| `NekoUI/NekoUI/Widget/layout/Stack.hpp` | 新建 | `VStack` / `HStack` / `ZStack` 自动布局容器 |
| `NekoUI/NekoUI/Engine/Engine.hpp` | 修改 | 引擎：根控件列表替代单 widget，`add<T>()` 模板 |
| `NekoUI/NekoUI/Engine/Engine.cpp` | 修改 | 引擎实现：遍历根控件，事件路由至所有根 |
| `NekoUI/NekoUI/Widget/Component/Button.hpp` | 修改 | Button：删除过时的纯虚覆盖，适配新基类 bounds |
| `NekoUI\main.cpp` | 修改 | 使用 `engine.add<T>()` 替代 `set_widget` |
| `NekoUI/NekoUI.vcxproj` | 修改 | 添加新 header 引用 |

---

### Task 1: Widget 基类重构

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Widget.hpp`
- No compile test until other tasks adapt (header-only change)

**Interfaces:**
- Consumes: current `neko::widget::Widget` signatures
- Produces: new `Widget` with `m_parent`, `m_children`, `m_bounds`, `m_z_order`, `m_dirty`, `m_visible`, `register_child`/`unregister_child` (private, `Sub` friend), `Constraints` struct

- [ ] **Step 1: 在 `Widget.hpp` 中添加 `Constraints` 结构体和完整的新 Widget 类**

将 `Widget.hpp` 全部替换为如下内容：

```cpp
#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../Backend/Backend.hpp"

namespace neko::widget {

    struct Constraints {
        int x = 0, y = 0;          // 可用区域左上角
        int width = INT_MAX;       // 可用宽度
        int height = INT_MAX;      // 可用高度
    };

    class Widget {
        template<typename T> friend class Sub;

    public:
        virtual ~Widget() = default;

        // === 生命周期 ===
        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void;
        virtual auto layout(Constraints constraints) -> void;
        virtual auto update(engine::Context& context) -> void;
        virtual auto handle_event(engine::Context& context, UINT msg,
                                  WPARAM wparam, LPARAM lparam) -> bool;

        // === 命中测试 ===
        [[nodiscard]] virtual auto hit_test(const neko::mouse::Mouse& mouse) const -> bool;

        // === Dirty 管理 ===
        auto mark_dirty() -> void;
        [[nodiscard]] auto dirty() const -> bool { return m_dirty; }
        auto clear_dirty() -> void { m_dirty = false; }

        // === 树结构（只读） ===
        [[nodiscard]] auto children() const -> const std::vector<Widget*>& { return m_children; }
        [[nodiscard]] auto parent() const -> Widget* { return m_parent; }
        [[nodiscard]] auto child_count() const -> size_t { return m_children.size(); }
        [[nodiscard]] auto root() -> Widget*;  // 向上追溯到根

        // === 坐标与尺寸 ===
        auto set_bounds(int x, int y, int w, int h) -> void;
        auto set_bounds(glm::ivec4 bounds) -> void;
        [[nodiscard]] auto bounds() const -> const glm::ivec4& { return m_bounds; }
        [[nodiscard]] auto x() const -> int { return m_bounds.x; }
        [[nodiscard]] auto y() const -> int { return m_bounds.y; }
        [[nodiscard]] auto width() const -> int { return m_bounds.z; }
        [[nodiscard]] auto height() const -> int { return m_bounds.w; }

        // === Z-Order ===
        auto set_z_order(int order) -> void { m_z_order = order; }
        [[nodiscard]] auto z_order() const -> int { return m_z_order; }

        // === 可见性 ===
        auto set_visible(bool v) -> void;
        [[nodiscard]] auto visible() const -> bool { return m_visible; }

    protected:
        Widget() = default;

    private:
        auto register_child(Widget* child) -> void;
        auto unregister_child(Widget* child) -> void;

        // 排序辅助
        [[nodiscard]] auto children_sorted_asc() const -> std::vector<Widget*>;   // 按 z_order 升序
        [[nodiscard]] auto children_sorted_desc() const -> std::vector<Widget*>;  // 按 z_order 降序

        Widget* m_parent = nullptr;
        std::vector<Widget*> m_children;
        glm::ivec4 m_bounds{};     // x, y, width, height
        int m_z_order = 0;
        bool m_dirty = true;
        bool m_visible = true;
    };

    // === inline 实现 ===

    inline auto Widget::draw(engine::Context& context, backend::Backend& backend) -> void {
        for (auto* child : children_sorted_asc()) {
            child->draw(context, backend);
        }
        m_dirty = false;
    }

    inline auto Widget::layout(Constraints /*constraints*/) -> void {
        // 默认空实现——子类按需覆盖来定位子控件
    }

    inline auto Widget::update(engine::Context& context) -> void {
        for (auto* child : m_children) {
            child->update(context);
        }
    }

    inline auto Widget::handle_event(engine::Context& context, UINT msg,
                                     WPARAM wparam, LPARAM lparam) -> bool
    {
        // 事件从最上层（高 z_order）子控件开始分发
        for (auto* child : children_sorted_desc()) {
            if (child->handle_event(context, msg, wparam, lparam)) {
                return true;
            }
        }
        // 子控件不处理，检查鼠标事件自己是否命中
        switch (msg) {
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
                if (hit_test(context.mouse)) {
                    return true;  // 命中了自己，子类按需覆盖
                }
                return false;
            default:
                return false;
        }
    }

    inline auto Widget::hit_test(const neko::mouse::Mouse& mouse) const -> bool {
        if (!m_visible) return false;
        return mouse.is_inside(m_bounds);
    }

    inline auto Widget::mark_dirty() -> void {
        m_dirty = true;
        if (m_parent) {
            m_parent->mark_dirty();  // 向上传播
        }
    }

    inline auto Widget::root() -> Widget* {
        auto* cur = this;
        while (cur->m_parent) {
            cur = cur->m_parent;
        }
        return cur;
    }

    inline auto Widget::set_bounds(int x, int y, int w, int h) -> void {
        m_bounds = {x, y, w, h};
        mark_dirty();
    }

    inline auto Widget::set_bounds(glm::ivec4 bounds) -> void {
        m_bounds = bounds;
        mark_dirty();
    }

    inline auto Widget::set_visible(bool v) -> void {
        m_visible = v;
        mark_dirty();
    }

    inline auto Widget::register_child(Widget* child) -> void {
        child->m_parent = this;
        m_children.push_back(child);
        mark_dirty();
    }

    inline auto Widget::unregister_child(Widget* child) -> void {
        auto it = std::ranges::find(m_children, child);
        if (it != m_children.end()) {
            m_children.erase(it);
        }
        child->m_parent = nullptr;
        mark_dirty();
    }

    inline auto Widget::children_sorted_asc() const -> std::vector<Widget*> {
        auto sorted = m_children;
        std::ranges::sort(sorted, std::ranges::less{}, &Widget::z_order);
        return sorted;
    }

    inline auto Widget::children_sorted_desc() const -> std::vector<Widget*> {
        auto sorted = m_children;
        std::ranges::sort(sorted, std::ranges::greater{}, &Widget::z_order);
        return sorted;
    }

} // namespace neko::widget
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI/Widget/Widget.hpp
git commit -m "feat: Widget 基类重构 —— 树结构、bounds、z-order、dirty 传播"
```

---

### Task 2: Sub\<T\> 自动注册包装器

**Files:**
- Create: `NekoUI/NekoUI/Widget/Sub.hpp`

**Interfaces:**
- Consumes: `neko::widget::Widget` (from Task 1)
- Produces: `neko::widget::Sub<T>` template class

- [ ] **Step 1: 创建 `Sub.hpp`**

```cpp
#pragma once

#include "Widget.hpp"

namespace neko::widget {

    template<typename T>
    class Sub : public T {
        static_assert(std::is_base_of_v<Widget, T>);

    public:
        template<typename... Args>
        explicit Sub(Widget* parent, Args&&... args)
            : T(std::forward<Args>(args)...)
        {
            if (parent) {
                Widget::register_child(this);
            }
        }

        ~Sub() noexcept {
            if (this->m_parent) {
                Widget::unregister_child(this);
            }
        }

        Sub(const Sub&) = delete;
        auto operator=(const Sub&) -> Sub& = delete;
        Sub(Sub&&) = default;
        auto operator=(Sub&&) -> Sub& = default;
    };

} // namespace neko::widget
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI/Widget/Sub.hpp
git commit -m "feat: 添加 Sub<T> 自动注册包装器"
```

---

### Task 3: 引擎层改动 —— 根控件列表

**Files:**
- Modify: `NekoUI/NekoUI/Engine/Engine.hpp`
- Modify: `NekoUI/NekoUI/Engine/Engine.cpp`

**Interfaces:**
- Consumes: `neko::widget::Widget` (Task 1)
- Produces: `Engine::add<T>()`, removes `Engine::set_widget()`

- [ ] **Step 1: 修改 `Engine.hpp`**

将 `set_widget` 替换为 `add<T>`，`shared_ptr<widget::Widget> widget` 替换为 `vector<unique_ptr<widget::Widget>>`：

```cpp
#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <vector>

#include "Context.hpp"

#include "../Type.hpp"
#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    class Engine final {
    public:
        explicit Engine(HWND hwnd);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;

        //! @brief 添加根控件
        template<typename T, typename... Args>
        requires std::derived_from<T, widget::Widget>
        auto add(Args&&... args) -> T& {
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            ptr->set_z_order(static_cast<int>(m_root_widgets.size()));
            auto& ref = *ptr;
            m_root_widgets.push_back(std::move(ptr));
            rebuild();
            return ref;
        }

        //! @brief 触发重建
        auto rebuild() -> void;

        //! @brief 投递窗口消息到消息队列（满时阻塞等待）
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;
    private:
        // ... 保持不变：render_loop, render_wait, render_frame,
        //     msg_loop, msg_dequeue, msg_dispatch, anim_inc, anim_dec ...

        backend::Backend backend;
        Context context{};

        // ... 线程/同步相关成员保持不变 ...

        std::vector<std::unique_ptr<widget::Widget>> m_root_widgets;
    };
} // namespace neko::engine
```

具体修改：
1. 删除 `auto set_widget(std::shared_ptr<widget::Widget> widget) -> void;`
2. 添加 `template<typename T, typename... Args> requires std::derived_from<T, widget::Widget> auto add(Args&&... args) -> T&;`
3. 删除 `std::shared_ptr<widget::Widget> widget;` 成员
4. 添加 `std::vector<std::unique_ptr<widget::Widget>> m_root_widgets;`

- [ ] **Step 2: 修改 `Engine.cpp`**

```cpp
// 删除 set_widget 实现
// 修改 render_frame:
auto Engine::render_frame() -> void {
    if (resize_pending.exchange(false)) {
        backend.resize(resize_size);
    }

    backend.begin();
    for (auto& root : m_root_widgets) {
        root->layout({0, 0, resize_size.x, resize_size.y});
        root->draw(context, backend);
    }
    backend.end();
    context.dirty = false;
}

// 修改 msg_loop 中 update 部分:
// 原来的:
// if (widget) {
//     widget->update(context);
// }
// 改为:
for (auto& root : m_root_widgets) {
    root->update(context);
}

// 修改 msg_dispatch 中 default 部分:
// 原来的:
// if (widget) {
//     widget->handle_event(context, msg, wparam, lparam);
// }
// 改为:
for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
    if ((*it)->handle_event(context, msg, wparam, lparam)) {
        break;
    }
}
```

完整替换 `Engine.cpp`：

```cpp
// 2026-07-02 00:23:43 (updated for retained mode)

#include "Engine.hpp"

#include "../Widget/Component/Animation.hpp"

namespace neko::engine {
    Engine::Engine(const HWND hwnd) : backend(hwnd) {
        context.rebuild = std::bind(&Engine::rebuild, this);
        context.rerender = std::bind(&std::condition_variable::notify_one, &render_notify);
        context.animation_start = std::bind(&Engine::anim_inc, this);
        context.animation_end = std::bind(&Engine::anim_dec, this);
        context.dpi_scale = backend.get_dpi_scale();

        msg_thread = std::jthread(std::bind(&Engine::msg_loop, this));
        render_thread = std::jthread(std::bind(&Engine::render_loop, this));
    }

    Engine::~Engine() {
        if (render_thread.joinable()) {
            render_thread.request_stop();
            render_notify.notify_one();
            render_thread.join();
        }
        if (msg_thread.joinable()) {
            msg_thread.request_stop();
            msg_notify.notify_one();
            msg_thread.join();
        }
    }

    auto Engine::rebuild() -> void {
        pending = true;
        render_notify.notify_one();
    }

    auto Engine::push_msg(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> void {
        std::unique_lock lock(msg_mutex);
        msg_space.wait(lock,
                       [this] -> bool {
                           return msg_count < MSG_QUEUE_MAX;
                       });

        msg_queue[msg_tail] = MsgEvent{msg, wparam, lparam};
        msg_tail = (msg_tail + 1) % MSG_QUEUE_MAX;
        ++msg_count;

        msg_notify.notify_one();
    }

    auto Engine::render_loop() -> void {
        while (!render_thread.get_stop_token().stop_requested()) {
            render_wait();
            if (render_thread.get_stop_token().stop_requested()) {
                break;
            }
            render_frame();
        }
    }

    auto Engine::render_wait() -> void {
        if (animation != 0) {
            return;
        }
        std::unique_lock lock(render_lock);
        render_notify.wait(lock,
                           [this] -> bool {
                               return render_thread.get_stop_token().stop_requested() || pending || context.dirty;
                           });
        pending = false;
    }

    auto Engine::render_frame() -> void {
        if (resize_pending.exchange(false)) {
            backend.resize(resize_size);
        }

        backend.begin();
        for (auto& root : m_root_widgets) {
            root->layout({0, 0, resize_size.x, resize_size.y});
            root->draw(context, backend);
        }
        backend.end();
        context.dirty = false;
    }

    auto Engine::msg_loop() -> void {
        while (!msg_thread.get_stop_token().stop_requested()) {
            const auto ev = msg_dequeue();
            if (!ev.has_value()) {
                break;
            }

            const auto [msg, wparam, lparam] = *ev;
            context.mouse.handle(msg, wparam, lparam);
            context.keyboard.handle(msg, wparam, lparam);
            msg_dispatch(msg, wparam, lparam);

            for (auto& root : m_root_widgets) {
                root->update(context);
            }
            if (context.dirty) {
                rebuild();
            }
        }
    }

    auto Engine::msg_dequeue() -> std::optional<MsgEvent> {
        std::unique_lock lock(msg_mutex);
        msg_notify.wait(lock,
                        [this] -> bool {
                            return msg_thread.get_stop_token().stop_requested() || msg_count > 0;
                        });
        if (msg_thread.get_stop_token().stop_requested()) {
            return std::nullopt;
        }

        const auto ev = msg_queue[msg_head];
        msg_head = (msg_head + 1) % MSG_QUEUE_MAX;
        --msg_count;
        lock.unlock();
        msg_space.notify_one();
        return ev;
    }

    auto Engine::msg_dispatch(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> void {
        switch (msg) {
            case WM_SIZE:
                resize_size = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                resize_pending.store(true, std::memory_order_release);
                context.dirty = true;
                break;
            case WM_DPICHANGED: {
                const UINT dpi = LOWORD(wparam);
                backend.set_dpi(dpi);
                context.dpi_scale = static_cast<float>(dpi) / 96.0F;
                context.dirty = true;
                break;
            }
            default:
                for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
                    if ((*it)->handle_event(context, msg, wparam, lparam)) {
                        break;
                    }
                }
                break;
        }
    }

    auto Engine::anim_inc() -> void {
        ++animation;
    }

    auto Engine::anim_dec() -> void {
        --animation;
    }
} // namespace neko::engine
```

- [ ] **Step 3: 提交**

```bash
git add NekoUI/NekoUI/Engine/Engine.hpp NekoUI/NekoUI/Engine/Engine.cpp
git commit -m "feat: 引擎层——根控件列表替代单 widget，add<T>() 模板"
```

---

### Task 4: Button 迁移适配

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Component/Button.hpp`

- [ ] **Step 1: 适配 Button**

主要改动：
1. 删除 `child_count()` 和 `dirty()` 覆盖（基类已提供）
2. 使用基类 `bounds()` / `set_bounds()` 替代自有 `rect_`
3. 在构造时调用 `set_bounds` 设置初始位置
4. `handle_event` 被移除——Button 事件响应在 `update()` 中已有（靠 `update` 中的鼠标状态判断），不需要 `handle_event` 覆盖。基类 `handle_event` 默认处理了子控件事件路由。

```cpp
#pragma once
#include "Animation.hpp"
#include "../Widget.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(const glm::ivec4 rect, std::string label = "")
            : label_(std::move(label))
        {
            set_bounds(rect);
        }

        //! @brief 处理数据状态（msg 线程）
        auto update(engine::Context& context) -> void override {
            const float s = context.dpi_scale;
            const bool hover = context.mouse.is_inside(bounds(), s);
            const bool down = hover && context.mouse.left_down;

            if (context.mouse.left_released() && hover && on_click) {
                on_click();
            }

            glm::vec4 target;
            if (down) {
                target = press_f;
            } else if (hover) {
                target = hover_f;
            } else {
                target = idle_f;
            }

            if (target != current_target_) {
                current_target_ = target;
                if (anim_state == AnimState::Idle) {
                    context.animation_start();
                    anim_state = AnimState::Animating;
                }
                color_anim = target;
                context.dirty = true;
            }

            // 更新动画
            Widget::update(context);
        }

        //! @brief 渲染（render 线程）
        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            const glm::vec4 current_f = color_anim();

            if (color_anim.is_done() && anim_state == AnimState::Animating) {
                context.animation_end();
                anim_state = AnimState::Idle;
            }
            const type::Color current{
                static_cast<int>(current_f.r * 255.0F + 0.5F),
                static_cast<int>(current_f.g * 255.0F + 0.5F),
                static_cast<int>(current_f.b * 255.0F + 0.5F),
                static_cast<int>(current_f.a * 255.0F + 0.5F),
            };
            backend.draw_rect_fill(bounds(), current);
            backend.draw_rect(bounds(), border_color, 2);

            if (!label_.empty()) {
                const auto& b = bounds();
                const int y_center = b.y + (b.w - 16) / 2;
                backend.draw_text(label_, {b.x + 8, y_center}, text_color);
            }

            Widget::draw(context, backend);
        }

        std::function<void()> on_click;
    private:
        enum class AnimState : std::uint8_t { Idle, Animating };

        std::string label_;

        glm::vec4 idle_f{100.0F / 255.0F, 130.0F / 255.0F, 180.0F / 255.0F, 1.0F};
        glm::vec4 hover_f{130.0F / 255.0F, 160.0F / 255.0F, 210.0F / 255.0F, 1.0F};
        glm::vec4 press_f{200.0F / 255.0F, 100.0F / 255.0F, 100.0F / 255.0F, 1.0F};

        glm::vec4 current_target_{idle_f};
        AnimState anim_state = AnimState::Idle;

        animation::EaseOutQuadAnimation<glm::vec4> color_anim{idle_f, 200};

        type::Color border_color{60, 80, 120, 255};
        type::Color text_color{220, 220, 230, 255};
    };
} // namespace neko::widget
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI/Widget/Component/Button.hpp
git commit -m "refactor: Button 迁移到新 Widget 基类，使用 set_bounds/bounds 替代自有的 rect_"
```

---

### Task 5: main.cpp 更新

**Files:**
- Modify: `NekoUI/main.cpp`

- [ ] **Step 1: 修改 `main.cpp`**

```cpp
#include <Windows.h>
#include <iostream>
#include <print>
#include <string>

#include "NekoUI/Engine/Engine.hpp"
#include "NekoUI/Widget/Component/Button.hpp"

namespace {
    auto msg_proc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> LRESULT {
        switch (msg) {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_GETMINMAXINFO: {
                auto* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
                mmi->ptMinTrackSize = {.x = 200, .y = 150};
                return 0;
            }
            default:
                break;
        }

        if (auto* engine = reinterpret_cast<neko::engine::Engine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
            switch (msg) {
                case WM_SIZE:
                case WM_DPICHANGED:
                case WM_MOUSEMOVE:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEWHEEL:
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_CHAR:
                    engine->push_msg(msg, wparam, lparam);
                    break;
                default: ;
            }
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
} // namespace

auto main(int argc, char* argv[]) -> int try {
    constexpr std::wstring class_name = L"NekoUI";

    WNDCLASSW win_class{};
    win_class.lpszClassName = class_name.data();
    win_class.hInstance = GetModuleHandleW(nullptr);
    win_class.lpfnWndProc = msg_proc;
    win_class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    win_class.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClassW(&win_class) == 0U) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    const HWND hwnd = CreateWindowW(class_name.data(), L"NekoUI", WS_OVERLAPPEDWINDOW,
                                    CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                                    nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    neko::engine::Engine engine(hwnd);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&engine));

    // 保留模式：使用 engine.add<T>() 扁平注册
    auto& btn = engine.add<neko::widget::Button>(glm::ivec4{100, 100, 200, 50}, "点我");
    btn.on_click = [] -> void {
        std::println("[NekoUI] Button clicked!\n");
    };

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) != 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
} catch (const std::exception& error) {
    std::cout << error.what();
}
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/main.cpp
git commit -m "refactor: main.cpp 使用 engine.add<T>() 替换 set_widget"
```

---

### Task 6: 布局容器（VStack / HStack / ZStack）

**Files:**
- Create: `NekoUI/NekoUI/Widget/layout/Stack.hpp`

- [ ] **Step 1: 创建 `Stack.hpp`**

```cpp
#pragma once

#include "../Widget.hpp"

namespace neko::widget::layout {

    // === VStack: 垂直排列子控件 ===
    class VStack : public Widget {
    public:
        explicit VStack(int spacing = 4) : m_spacing(spacing) {}

        auto layout(Constraints constraints) -> void override {
            int current_y = constraints.y;
            for (auto* child : children()) {
                child->set_bounds(constraints.x, current_y,
                                  constraints.width, child->height());
                current_y += child->height() + m_spacing;
                // 给子控件传递布局约束
                child->layout({constraints.x, 0, constraints.width, child->height()});
            }
        }

    private:
        int m_spacing = 4;
    };

    // === HStack: 水平排列子控件 ===
    class HStack : public Widget {
    public:
        explicit HStack(int spacing = 4) : m_spacing(spacing) {}

        auto layout(Constraints constraints) -> void override {
            int current_x = constraints.x;
            for (auto* child : children()) {
                child->set_bounds(current_x, constraints.y,
                                  child->width(), constraints.height);
                current_x += child->width() + m_spacing;
                child->layout({0, constraints.y, child->width(), constraints.height});
            }
        }

    private:
        int m_spacing = 4;
    };

    // === ZStack: 层叠排列子控件（重叠，按 z_order 绘制） ===
    class ZStack : public Widget {
    public:
        auto layout(Constraints constraints) -> void override {
            for (auto* child : children()) {
                child->set_bounds(constraints.x, constraints.y,
                                  constraints.width, constraints.height);
                child->layout(constraints);
            }
        }
    };

} // namespace neko::widget::layout
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI/Widget/layout/Stack.hpp
git commit -m "feat: 添加 VStack/HStack/ZStack 自动布局容器"
```

---

### Task 7: vcxproj 添加新 header 引用

**Files:**
- Modify: `NekoUI/NekoUI.vcxproj`

- [ ] **Step 1: 在 `NekoUI.vcxproj` 的 `ClInclude` ItemGroup 中添加新文件**

```xml
<ClInclude Include="NekoUI\Widget\Sub.hpp" />
<ClInclude Include="NekoUI\Widget\layout\Stack.hpp" />
```

添加到现有的 `ItemGroup` 中（在 `Button.hpp` 后面）。

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI.vcxproj
git commit -m "chore: vcxproj 添加 Sub.hpp 和 Stack.hpp"
```

---

### Task 8: 构建验证

**Files:**
- Build output

- [ ] **Step 1: 构建项目**

```bash
msbuild NekoUI.slnx /p:Configuration=Debug /p:Platform=x64
```

预期结果：编译成功，无错误。可能会有的警告：
- `std::println` 可用性（Intel C++ 2026 对 C++26 支持度）
- 未使用参数警告（可忽略或在函数签名中去掉参数名）

- [ ] **Step 2: 如果有编译错误，修复后重新构建**

常见问题排查：
- `glm/glm.hpp` 找不到 → 确认 `../Library/glm/` 存在
- `std::ranges::find` / `std::ranges::sort` 不可用 → 替换为 `std::find` / `std::sort`
- `std::println` 不可用 → 暂时用 `OutputDebugStringW` 或 `printf` 替代
- `std::derived_from` 不可用 → 去掉 requires 子句，用 `static_assert` 替代

- [ ] **Step 3: 提交最终修复**

```bash
git add -A
git commit -m "fix: 编译修复"
```
