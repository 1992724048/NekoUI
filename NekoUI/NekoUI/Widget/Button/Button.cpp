#include "Button.hpp"

#include <limits>
#include <utility>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Platform/Event.hpp"

namespace neko::widget {
    Button::Button(engine::Context&, std::string text, std::function<void()> on_click) :
        text_(std::move(text)),
        on_click_(std::move(on_click)) {}

    auto Button::layout(Vec4I available, engine::Context& /*context*/) -> void {
        auto effective = available;
        const auto use_parent = size_.size.x == std::numeric_limits<float>::max() || size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(size_.size.x);
            effective.w = effective.y + static_cast<int>(size_.size.y);
        }
        set_bounds(effective);
    }

    auto Button::draw(Vec4I /*rect*/, engine::Context& context, backend::Backend& backend) -> Rect {
        auto bg = background_.color.value != 0 ? background_ : style::Background{context.scheme.primary};
        auto tc = text_color_.value != 0 ? text_color_ : Color{0xFFFFFFFF};

        backend.draw_rect_fill(bounds, bg.color);
        if (border_.size > 0.0F) {
            backend.draw_rect(bounds, border_.color, static_cast<int>(border_.size));
        }
        if (!text_.empty()) {
            const auto text_pos = Vec2I{
                .x = bounds.x + (bounds.z - bounds.x) / 10,
                .y = bounds.y + (bounds.w - bounds.y) / 2
            };
            backend.draw_text(text_, text_pos, tc, font_size_);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
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

    auto Button::on_click(std::function<void()> cb) -> Button& {
        on_click_ = std::move(cb);
        return *this;
    }
} // namespace neko::widget
