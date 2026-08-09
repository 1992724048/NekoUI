// 2026-08-02 04:26:20

#include "Button.hpp"

#include <limits>
#include <utility>

#include "../../Backend/DirectX11/DirectX11.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Platform/Event.hpp"

namespace neko::widget {
    Button::Button(const engine::Context& context, std::string text, std::function<void()> on_click) :
        text_(std::move(text)),
        on_click_(std::move(on_click)) {
        scale_.bind(context.anim_inc, context.anim_dec);
    }

    auto Button::layout(const Vec4I available, engine::Context& /*context*/) -> void {
        auto effective = available;
        const auto use_parent = size_.size.x == std::numeric_limits<float>::max() || size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(size_.size.x);
            effective.w = effective.y + static_cast<int>(size_.size.y);
        }
        set_bounds(effective);
    }

    auto Button::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        const auto bg = background_.color.value != 0 ? background_ : style::Background{hover_ ? context.scheme.secondary_container : context.scheme.primary};
        const auto tc = text_color_.value != 0 ? text_color_ : Color{0xFFFFFFFF};

        const auto s = scale_.tick();
        const auto center_x = (bounds.x + bounds.z) / 2;
        const auto center_y = (bounds.y + bounds.w) / 2;
        const auto half_w = static_cast<int>(static_cast<float>(bounds.z - bounds.x) * s / 2.0F);
        const auto half_h = static_cast<int>(static_cast<float>(bounds.w - bounds.y) * s / 2.0F);
        const Vec4I visual{.x = center_x - half_w, .y = center_y - half_h, .z = center_x + half_w, .w = center_y + half_h};

        backend.draw_rect_fill(visual, bg.color);
        if (border_.size > 0.0F) {
            backend.draw_rect(visual, border_.color, static_cast<int>(border_.size));
        }
        if (!text_.empty()) {
            const auto text_pos = Vec2I{.x = visual.x + (visual.z - visual.x) / 10, .y = visual.y + (visual.w - visual.y) / 2};
            backend.draw_text(text_, text_pos, tc, font_size_);
        }
        return {.x = visual.x, .y = visual.y, .width = visual.z - visual.x, .height = visual.w - visual.y};
    }

    auto Button::build(engine::Context& context) -> void {}

    auto Button::event(engine::Context& context) -> void {}

    auto Button::input(engine::Context& context, const platform::Event& event) -> void {
        if (std::get_if<device::MouseMoveEvent>(&event) != nullptr) {
            const auto mouse = context.mouse.lock();
            if (!mouse) {
                return;
            }
            if (const auto inside = mouse->is_inside(bounds); inside != hover_) {
                hover_ = inside;
                scale_.to_value(hover_ ? 1.06F : 1.0F);
                if (context.mark_dirty) {
                    context.mark_dirty();
                }
            }
            return;
        }

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
