# Task 1: Widget 基类重构

**Location in project:** This is the foundational task — every other task depends on it. Changes the core Widget base class to support tree structure and retained mode.

**File to modify:**
- `NekoUI/NekoUI/Widget/Widget.hpp`

**What to do:**
Replace the entire `Widget.hpp` with the new version that adds:
1. `Constraints` struct (x, y, width, height available area)
2. Tree structure: `m_parent`, `m_children`, `register_child()`/`unregister_child()` (private, `friend class Sub`)
3. Coordinate system: `m_bounds` (glm::ivec4), `set_bounds()`, `bounds()`, `x()`, `y()`, `width()`, `height()`
4. Z-order: `m_z_order`, `set_z_order()`, `z_order()`
5. Dirty propagation: `m_dirty`, `mark_dirty()` (propagates to parent), `dirty()`, `clear_dirty()`
6. Visibility: `m_visible`, `set_visible()`
7. Virtual `hit_test(const neko::mouse::Mouse&)` — default: `is_inside(bounds)`
8. Default `draw()` iterates children sorted by z_order ascending
9. Default `update()` iterates children
10. Default `handle_event()` tries children (z_order descending first, then self hit-test)
11. Default `layout()` empty (subclass override)
12. Helper sort methods: `children_sorted_asc()`, `children_sorted_desc()`

## Exact code to write

```cpp
#pragma once

#include <vector>
#include <glm/glm.hpp>

#include "../Backend/Backend.hpp"

namespace neko::widget {

    struct Constraints {
        int x = 0, y = 0;
        int width = INT_MAX;
        int height = INT_MAX;
    };

    class Widget {
        template<typename T> friend class Sub;

    public:
        virtual ~Widget() = default;

        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void;
        virtual auto layout(Constraints constraints) -> void;
        virtual auto update(engine::Context& context) -> void;
        virtual auto handle_event(engine::Context& context, UINT msg,
                                  WPARAM wparam, LPARAM lparam) -> bool;

        [[nodiscard]] virtual auto hit_test(const neko::mouse::Mouse& mouse) const -> bool;

        auto mark_dirty() -> void;
        [[nodiscard]] auto dirty() const -> bool { return m_dirty; }
        auto clear_dirty() -> void { m_dirty = false; }

        [[nodiscard]] auto children() const -> const std::vector<Widget*>& { return m_children; }
        [[nodiscard]] auto parent() const -> Widget* { return m_parent; }
        [[nodiscard]] auto child_count() const -> size_t { return m_children.size(); }
        [[nodiscard]] auto root() -> Widget*;

        auto set_bounds(int x, int y, int w, int h) -> void;
        auto set_bounds(glm::ivec4 bounds) -> void;
        [[nodiscard]] auto bounds() const -> const glm::ivec4& { return m_bounds; }
        [[nodiscard]] auto x() const -> int { return m_bounds.x; }
        [[nodiscard]] auto y() const -> int { return m_bounds.y; }
        [[nodiscard]] auto width() const -> int { return m_bounds.z; }
        [[nodiscard]] auto height() const -> int { return m_bounds.w; }

        auto set_z_order(int order) -> void { m_z_order = order; }
        [[nodiscard]] auto z_order() const -> int { return m_z_order; }

        auto set_visible(bool v) -> void;
        [[nodiscard]] auto visible() const -> bool { return m_visible; }

    protected:
        Widget() = default;

    private:
        auto register_child(Widget* child) -> void;
        auto unregister_child(Widget* child) -> void;
        [[nodiscard]] auto children_sorted_asc() const -> std::vector<Widget*>;
        [[nodiscard]] auto children_sorted_desc() const -> std::vector<Widget*>;

        Widget* m_parent = nullptr;
        std::vector<Widget*> m_children;
        glm::ivec4 m_bounds{};
        int m_z_order = 0;
        bool m_dirty = true;
        bool m_visible = true;
    };

    // === inline implementations ===

    inline auto Widget::draw(engine::Context& context, backend::Backend& backend) -> void {
        for (auto* child : children_sorted_asc()) {
            child->draw(context, backend);
        }
        m_dirty = false;
    }

    inline auto Widget::layout(Constraints /*constraints*/) -> void {}

    inline auto Widget::update(engine::Context& context) -> void {
        for (auto* child : m_children) {
            child->update(context);
        }
    }

    inline auto Widget::handle_event(engine::Context& context, UINT msg,
                                     WPARAM wparam, LPARAM lparam) -> bool
    {
        for (auto* child : children_sorted_desc()) {
            if (child->handle_event(context, msg, wparam, lparam)) {
                return true;
            }
        }
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
                    return true;
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
            m_parent->mark_dirty();
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

## Global Constraints
- Trailing return types: `auto foo() -> void`
- Namespace: `neko::widget`
- Bounds: `glm::ivec4(x, y, width, height)` — not x1/y1/x2/y2

## Verification
This is a header-only change. No compile test possible until later tasks adapt the code that uses Widget. Verify correctness by reading the file once written.

## Commit
```bash
git add NekoUI/NekoUI/Widget/Widget.hpp
git commit -m "feat: Widget 基类重构 —— 树结构、bounds、z-order、dirty 传播"
```
