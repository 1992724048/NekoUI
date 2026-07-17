#include "Button.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Platform/Event.hpp"

namespace neko::widget {
    Button::Button(engine::Context&, std::string text, std::function<void()> on_click) :
        text_(std::move(text)),
        on_click_(std::move(on_click)) {}

    auto Button::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        const auto& s = style_;

        // 使用 rect 参数（线程局部），不修改 bounds（避免渲染/输入线程数据竞争）
        auto effective = rect;
        if (const auto use_parent = s.size.x == std::numeric_limits<float>::max() || s.size.y == std::numeric_limits<float>::max(); !use_parent) {
            effective.z = effective.x + static_cast<int>(s.size.x);
            effective.w = effective.y + static_cast<int>(s.size.y);
        }

        backend.draw_rect_fill(effective, s.background_color);

        if (s.border_size > 0.0F) {
            backend.draw_rect(effective, s.border_color, static_cast<int>(s.border_size));
        }

        if (!text_.empty()) {
            const auto font_size = s.font_size;
            const auto text_pos = Vec2I{
                .x = static_cast<int>(effective.x + static_cast<float>(effective.z) * 0.1F),
                .y = static_cast<int>(effective.y + static_cast<float>(effective.w) * 0.5F)
            };
            backend.draw_text(text_, text_pos, s.text_color, font_size);
        }

        return Rect{.x = effective.x, .y = effective.y, .width = effective.z - effective.x, .height = effective.w - effective.y};
    }

    auto Button::build(engine::Context& context) -> void {}

    auto Button::event(engine::Context& context) -> void {}

    auto Button::input(engine::Context& context, const platform::Event& event) -> void {
        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            if (on_click_ && context.mouse.lock()) {
                if (context.mouse.lock()->is_inside(bounds)) {
                    on_click_();
                }
            }
        }
    }

    auto Button::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }

    auto Button::style(const ButtonStyle& s) -> Button& {
        style_ = s;
        return *this;
    }

    auto Button::on_click(std::function<void()> cb) -> Button& {
        on_click_ = std::move(cb);
        return *this;
    }

    auto Button::text(std::string t) -> Button& {
        text_ = std::move(t);
        return *this;
    }
} // namespace neko::widget
