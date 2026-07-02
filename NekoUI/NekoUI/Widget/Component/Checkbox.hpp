#pragma once

#include "Animation.hpp"
#include "../Widget.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Checkbox final : public Widget {
    public:
        explicit Checkbox(const glm::ivec4 bounds = {}, std::string label = "") : m_label(std::move(label)) {
            set_bounds(bounds);
            sync_anim();
        }

        auto update(engine::Context& context) -> void override {
            m_anim.update();
            Widget::update(context);
        }

        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            const auto& b = bounds();
            constexpr int box = 18;
            const int bx = b.x;
            const int by = b.y + (b.w - box) / 2;

            // Cache animated value
            const glm::vec4 current_f = m_anim();

            // Animation lifecycle
            if (m_anim.is_done() && m_prev_animating) {
                context.animation_end();
                m_prev_animating = false;
            }

            // Box background (animated color)
            const glm::ivec4 box_color{static_cast<int>(current_f.r * 255.0f + 0.5f), static_cast<int>(current_f.g * 255.0f + 0.5f), static_cast<int>(current_f.b * 255.0f + 0.5f), 255,};

            if (m_checked || m_anim.progress() > 0.0f) {
                backend.draw_rect_fill({bx, by, box, box}, box_color);
            }

            const auto border = has_focus() ? m_focus_border : m_normal_border;
            backend.draw_rect({bx, by, box, box}, border, 1);

            // 勾号（白色矩形近似）
            if (m_checked || m_anim.progress() > 0.0f) {
                backend.draw_rect_fill({bx + 4, by + 4, box - 8, box - 8}, m_check_color);
            }

            // 标签（垂直居中，baseline 偏移）
            if (!m_label.empty()) {
                constexpr int font_ascent = 13;
                const int label_y = b.y + (b.w - 16) / 2 + font_ascent;
                backend.draw_text(m_label, {bx + box + 8, label_y}, m_text_color);
            }

            // 焦点外框（全控件范围）
            if (has_focus()) {
                backend.draw_rect(b, m_focus_border, 1);
            }

            Widget::draw(context, backend);
        }

        auto handle_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool override {
            if (msg == WM_LBUTTONDOWN && hit_test(context.mouse)) {
                if (context.request_focus) {
                    context.request_focus(this);
                }
                toggle(context);
                return true;
            }
            if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
                toggle(context);
                return true;
            }
            return Widget::handle_event(context, msg, wparam, lparam);
        }

        auto focusable() const -> bool override {
            return true;
        }

        auto is_checked() const -> bool {
            return m_checked;
        }

        auto set_checked(const bool checked) -> void {
            if (m_checked == checked) {
                return;
            }
            m_checked = checked;
            sync_anim();
        }

        auto toggle() -> void {
            m_checked = !m_checked;
            sync_anim();
            if (on_toggled) {
                on_toggled(m_checked);
            }
        }

        std::function<void(bool)> on_toggled;
    private:
        auto toggle(engine::Context& context) -> void {
            m_checked = !m_checked;
            sync_anim();
            context.animation_start();
            m_prev_animating = true;
            context.dirty = true;
            if (on_toggled) {
                on_toggled(m_checked);
            }
        }

        auto sync_anim() -> void {
            if (m_checked) {
                m_anim = animation::EaseOutQuadAnimation<glm::vec4>{m_unchecked_color, 150};
                m_anim = m_checked_color;
            } else {
                m_anim = animation::EaseOutQuadAnimation<glm::vec4>{m_checked_color, 150};
                m_anim = m_unchecked_color;
            }
        }

        bool m_checked = false;
        bool m_prev_animating = false;
        std::string m_label;
        animation::EaseOutQuadAnimation<glm::vec4> m_anim{{0.7f, 0.7f, 0.75f, 1.0f}, 150};

        glm::vec4 m_unchecked_color{0.7f, 0.7f, 0.75f, 1.0f};
        glm::vec4 m_checked_color{0.24f, 0.47f, 0.86f, 1.0f};
        type::Color m_check_color{255, 255, 255, 255};
        type::Color m_text_color{30, 30, 30, 255};
        type::Color m_normal_border{140, 140, 150, 255};
        type::Color m_focus_border{60, 120, 220, 255};
    };
} // namespace neko::widget
