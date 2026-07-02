#pragma once

#include <algorithm>
#include <climits>
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
        template<typename T>
        friend class Sub;
        friend class neko::engine::Engine;
    public:
        virtual ~Widget() = default;

        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void;
        virtual auto layout(Constraints constraints) -> void;
        virtual auto update(engine::Context& context) -> void;
        virtual auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool;

        [[nodiscard]] virtual auto hit_test(const mouse::Mouse& mouse) const -> bool;

        auto mark_dirty() -> void;

        [[nodiscard]] auto dirty() const -> bool {
            return m_dirty;
        }

        auto clear_dirty() -> void {
            m_dirty = false;
        }

        [[nodiscard]] auto children() const -> const std::vector<Widget*>& {
            return m_children;
        }

        [[nodiscard]] auto parent() const -> Widget* {
            return m_parent;
        }

        [[nodiscard]] auto child_count() const -> size_t {
            return m_children.size();
        }

        [[nodiscard]] auto root() -> Widget*;

        auto set_bounds(int x, int y, int w, int h) -> void;
        auto set_bounds(glm::ivec4 bounds) -> void;

        [[nodiscard]] auto bounds() const -> const glm::ivec4& {
            return m_bounds;
        }

        [[nodiscard]] auto x() const -> int {
            return m_bounds.x;
        }

        [[nodiscard]] auto y() const -> int {
            return m_bounds.y;
        }

        [[nodiscard]] auto width() const -> int {
            return m_bounds.z;
        }

        [[nodiscard]] auto height() const -> int {
            return m_bounds.w;
        }

        auto set_z_order(const int order) -> void {
            m_z_order = order;
        }

        [[nodiscard]] auto z_order() const -> int {
            return m_z_order;
        }

        auto set_visible(bool v) -> void;

        [[nodiscard]] auto visible() const -> bool {
            return m_visible;
        }

        [[nodiscard]] virtual auto focusable() const -> bool {
            return false;
        }

        virtual auto on_focus_gained() -> void {}

        virtual auto on_focus_lost() -> void {}

        [[nodiscard]] auto has_focus() const -> bool {
            return m_has_focus;
        }
    protected:
        Widget() = default;

        bool m_has_focus = false;
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

    inline auto Widget::handle_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
        for (auto* child : children_sorted_desc()) {
            if (child->handle_event(context, msg, wparam, lparam)) {
                return true;
            }
        }
        switch (msg) {
            case WM_LBUTTONDOWN:
                if (context.mouse.is_inside(m_bounds, context.dpi_scale)) {
                    if (focusable()) {
                        if (context.request_focus) {
                            context.request_focus(this);
                        }
                    }
                    return true;
                }
                return false;
            case WM_MOUSEMOVE:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
                if (context.mouse.is_inside(m_bounds, context.dpi_scale)) {
                    return true;
                }
                return false;
            default:
                return false;
        }
    }

    inline auto Widget::hit_test(const mouse::Mouse& mouse) const -> bool {
        if (!m_visible) {
            return false;
        }
        return mouse.is_inside(m_bounds);
    }

    inline auto Widget::mark_dirty() -> void {
        m_dirty = true;
        if (m_parent != nullptr) {
            m_parent->mark_dirty();
        }
    }

    inline auto Widget::root() -> Widget* {
        auto* cur = this;
        while (cur->m_parent != nullptr) {
            cur = cur->m_parent;
        }
        return cur;
    }

    inline auto Widget::set_bounds(int x, int y, int w, int h) -> void {
        m_bounds = {x, y, w, h};
        mark_dirty();
    }

    inline auto Widget::set_bounds(const glm::ivec4 bounds) -> void {
        m_bounds = bounds;
        mark_dirty();
    }

    inline auto Widget::set_visible(const bool v) -> void {
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
            child->m_parent = nullptr;
            mark_dirty();
        }
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
