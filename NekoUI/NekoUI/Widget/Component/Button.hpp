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
