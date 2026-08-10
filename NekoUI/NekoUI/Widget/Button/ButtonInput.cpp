// 2026-08-10

#include "ButtonInput.hpp"

#include <utility>

#include "../../Device/Mouse.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    ButtonInput::ButtonInput(neko::widget::Widget& owner, const engine::Context& /*context*/, OnClick onClick) :
        InputBehavior{owner},
        onClick_(std::move(onClick)) {}

    auto ButtonInput::input(engine::Context& context, const platform::Event& event) -> void {
        if (std::get_if<device::MouseMoveEvent>(&event) != nullptr) {
            const auto mouse = context.mouse.lock();
            if (!mouse) {
                return;
            }
            const auto inside = mouse->is_inside(owner_.get_bounds());
            if (inside != owner_.get_hovered()) {
                owner_.set_hovered(inside);
                if (context.mark_dirty) {
                    context.mark_dirty();
                }
            }
            return;
        }

        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            const auto mouse = context.mouse.lock();
            if (onClick_ && mouse && mouse->is_inside(owner_.get_bounds())) {
                onClick_();
            }
        }
    }
} // namespace neko::behavior
