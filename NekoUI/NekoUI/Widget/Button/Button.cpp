#include "Button.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Platform/Event.hpp"

namespace neko::widget {
    Button::Button(std::string text, std::function<void()> on_click)
        : text_(std::move(text)), on_click_(std::move(on_click)) {}

    auto Button::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void {
        // 1. 绘制背景
        const auto bg_color = background_.color.value != 0 ? background_.color : type::Color{0xFF1E1E1E};

        backend.draw_rect_fill(bounds, bg_color);

        // 2. 绘制边框
        if (border_.size > 0.0f) {
            backend.draw_rect(bounds, border_.color, static_cast<int>(border_.size));
        }

        // 3. 绘制文字（居中）
        if (!text_.empty()) {
            const auto font_size = size_.size.x > 0.0f ? size_.size.x : 16.0f;
            const auto text_pos = type::Vec2I{
                static_cast<int>(bounds.x + static_cast<float>(bounds.z) * 0.1f),
                static_cast<int>(bounds.y + static_cast<float>(bounds.w) * 0.5f)
            };
            // 反转 RGB 但保持 alpha 不透明
            const auto text_color = type::Color{bg_color.value ^ 0x00FFFFFFu};
            backend.draw_text(text_, text_pos, text_color, font_size);
        }
    }

    auto Button::build(engine::Context& context) -> void {}

    auto Button::event(engine::Context& context) -> void {}

    auto Button::input(engine::Context& context, const platform::Event& event) -> void {
        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            if (on_click_) {
                on_click_();
            }
        }
    }

    auto Button::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
