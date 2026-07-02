#pragma once

#include "../Widget.hpp"
#include "../Component/Animation.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Checkbox final : public Widget {
    public:
        explicit Checkbox(glm::ivec4 bounds = {}, std::string label = "");

        auto update(engine::Context& context) -> void override;

        auto draw(engine::Context& context, backend::Backend& backend) -> void override;

        auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool override;

        [[nodiscard]] auto focusable() const -> bool override;

        [[nodiscard]] auto is_checked() const -> bool;

        auto set_checked(bool checked) -> void;

        auto toggle() -> void;

        std::function<void(bool)> on_toggled;
    private:
        auto toggle(engine::Context& context) -> void;

        auto sync_anim() -> void;

        bool m_checked = false;
        bool m_prev_animating = false;
        std::string m_label;
        animation::EaseOutQuadAnimation<glm::vec4> m_anim{{0.7f, 0.7f, 0.75f, 1.0f}, 150};

        glm::vec4 m_unchecked_color{0.7F, 0.7F, 0.75F, 1.0F};
        glm::vec4 m_checked_color{0.24F, 0.47F, 0.86F, 1.0F};
        type::Color m_check_color{255, 255, 255, 255};
        type::Color m_text_color{255, 255, 255, 255};
        type::Color m_normal_border{140, 140, 150, 255};
        type::Color m_focus_border{60, 120, 220, 255};
    };
} // namespace neko::widget
