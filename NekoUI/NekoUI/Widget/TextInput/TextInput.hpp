// 2026-07-02 08:06:31

#pragma once

#include "../Widget.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace neko::widget {
    class TextInput final : public Widget {
    public:
        explicit TextInput(glm::ivec4 bounds = {}, std::string placeholder = "");

        auto update(engine::Context& context) -> void override;

        auto draw(engine::Context& context, backend::Backend& backend) -> void override;

        auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool override;

        [[nodiscard]] auto focusable() const -> bool override;

        auto on_focus_gained() -> void override;

        auto on_focus_lost() -> void override;

        [[nodiscard]] auto text() const -> const std::string&;

        auto set_text(std::string_view t) -> void;

        auto set_placeholder(std::string_view t) -> void;

        std::function<void(std::string_view)> on_text_changed;
    private:
        [[nodiscard]] auto has_selection() const -> bool;

        auto delete_selection() -> void;

        std::string m_text;
        std::string m_placeholder;
        int m_cursor_pos = 0;
        int m_sel_start = -1;
        std::chrono::steady_clock::time_point m_cursor_tick;
        bool m_cursor_visible = true;

        type::Color m_bg_color{240, 240, 245, 255};
        type::Color m_text_color{30, 30, 30, 255};
        type::Color m_cursor_color{60, 60, 60, 255};
        type::Color m_sel_color{180, 200, 240, 255};
        type::Color m_border_color{180, 180, 190, 255};
        type::Color m_focus_border_color{60, 120, 220, 255};
        type::Color m_placeholder_color{180, 180, 180, 255};
    };
} // namespace neko::widget
