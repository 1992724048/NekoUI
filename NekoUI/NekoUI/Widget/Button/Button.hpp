#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(glm::ivec4 rect, std::string label = "");

        [[nodiscard]] auto wants_hand_cursor() const -> bool override {
            return true;
        }

        std::function<void()> on_click;
    protected:
        auto on_update(engine::Context& context) -> void override;
        auto on_animate(std::chrono::milliseconds dt) -> void override;
        auto on_draw(engine::Context& context, backend::Backend& backend) -> void override;
    private:
        std::string label_;

        glm::vec4 idle_f{100.0F / 255.0F, 130.0F / 255.0F, 180.0F / 255.0F, 1.0F};
        glm::vec4 hover_f{130.0F / 255.0F, 160.0F / 255.0F, 210.0F / 255.0F, 1.0F};
        glm::vec4 press_f{200.0F / 255.0F, 100.0F / 255.0F, 100.0F / 255.0F, 1.0F};

        state::AnimatedState<glm::vec4> fill_color_{idle_f, std::chrono::milliseconds(200), animation::ease_out_quad};
        glm::vec4 target_{idle_f};

        type::Color border_color_{60, 80, 120, 255};
        type::Color text_color_{220, 220, 230, 255};
    };
} // namespace neko::widget
