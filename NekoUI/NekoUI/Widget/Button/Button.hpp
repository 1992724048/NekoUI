#pragma once
#include "../Widget.hpp"
#include "../Component/Animation.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(glm::ivec4 rect, std::string label = "");

        //! @brief 处理数据状态（msg 线程）
        auto update(engine::Context& context) -> void override;

        //! @brief 渲染（render 线程）
        auto draw(engine::Context& context, backend::Backend& backend) -> void override;

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
