#pragma once
#include "Animation.hpp"
#include "../Widget.hpp"

#include <string>

namespace neko::widget {
    //! @brief 按钮控件（带颜色动画渐变）
    class Button final : public Widget {
    public:
        explicit Button(glm::ivec4 rect, std::string label = "") : rect_(rect),
                                                                   label_(std::move(label)) {}

        //! @brief 处理数据状态（msg 线程）
        auto update(engine::Context& context) -> void override {
            const auto mouse = context.mouse.pos;
            const bool hover = mouse.x >= rect_.x && mouse.x <= rect_.x + rect_.z && mouse.y >= rect_.y && mouse.y <= rect_.y + rect_.w;
            const bool down = hover && context.mouse.left_down;

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
            }

            color_anim = target;

            if (color_anim.is_done() && anim_state == AnimState::Animating) {
                context.animation_end();
                anim_state = AnimState::Idle;
            }
        }

        //! @brief 渲染（render 线程）
        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            const glm::vec4 current_f = color_anim();
            const type::Color current{
                static_cast<int>(current_f.r * 255.0F + 0.5F),
                static_cast<int>(current_f.g * 255.0F + 0.5F),
                static_cast<int>(current_f.b * 255.0F + 0.5F),
                static_cast<int>(current_f.a * 255.0F + 0.5F),
            };
            backend.draw_rect_fill(rect_, current);
            backend.draw_rect(rect_, border_color, 2);

            if (!label_.empty()) {
                const glm::ivec2 text_pos{rect_.x + 8, rect_.y + rect_.w / 2 - 8};
                backend.draw_text(label_, text_pos, text_color, 14.0F);
            }
        }

        std::function<void()> on_click;
    private:
        enum class AnimState { Idle, Animating };

        glm::ivec4 rect_;
        std::string label_;

        glm::vec4 idle_f{100.0F / 255.0F, 130.0F / 255.0F, 180.0F / 255.0F, 1.0F};
        glm::vec4 hover_f{130.0F / 255.0F, 160.0F / 255.0F, 210.0F / 255.0F, 1.0F};
        glm::vec4 press_f{200.0F / 255.0F, 100.0F / 255.0F, 100.0F / 255.0F, 1.0F};

        glm::vec4 current_target_{idle_f};
        AnimState anim_state = AnimState::Idle;

        animation::EaseOutQuadAnimation<glm::vec4> color_anim{idle_f, 120};

        type::Color border_color{60, 80, 120, 255};
        type::Color text_color{220, 220, 230, 255};
    };
} // namespace neko::widget
