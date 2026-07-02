#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <vector>

#include <glm/glm.hpp>

#include "../Backend/Backend.hpp"
#include "../Engine/Context.hpp"
#include "State/State.hpp"

namespace neko::widget {
    struct Constraints {
        int x = 0, y = 0;
        int width = INT_MAX;
        int height = INT_MAX;
    };

    class Widget {
        template<typename T>
        friend class Sub;
        friend class engine::Engine;
    public:
        virtual ~Widget();

        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void;
        virtual auto layout(Constraints constraints) -> void;
        virtual auto update(engine::Context& context) -> void;
        virtual auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool;

        [[nodiscard]] virtual auto hit_test(const mouse::Mouse& mouse) const -> bool;

        auto mark_dirty() -> void;
        [[nodiscard]] auto dirty() const -> bool;
        auto clear_dirty() -> void;

        [[nodiscard]] auto children() const -> const std::vector<Widget*>&;
        [[nodiscard]] auto parent() const -> Widget*;
        [[nodiscard]] auto child_count() const -> size_t;
        [[nodiscard]] auto root() -> Widget*;

        auto set_bounds(int x, int y, int w, int h) -> void;
        auto set_bounds(glm::ivec4 bounds) -> void;
        [[nodiscard]] auto bounds() const -> const glm::ivec4&;
        [[nodiscard]] auto x() const -> int;
        [[nodiscard]] auto y() const -> int;
        [[nodiscard]] auto width() const -> int;
        [[nodiscard]] auto height() const -> int;

        auto set_z_order(int order) -> void;
        [[nodiscard]] auto z_order() const -> int;

        auto set_visible(bool v) -> void;
        [[nodiscard]] auto visible() const -> bool;

        [[nodiscard]] virtual auto focusable() const -> bool;
        virtual auto on_focus_gained() -> void;
        virtual auto on_focus_lost() -> void;
        [[nodiscard]] auto has_focus() const -> bool;
    protected:
        Widget();

        template<typename T>
        auto bind_state(neko::state::State<T>& state) -> void {
            state.set_observer([this](const T&) { mark_dirty(); });
        }

        virtual auto animate(std::chrono::milliseconds dt) -> void;

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
} // namespace neko::widget
