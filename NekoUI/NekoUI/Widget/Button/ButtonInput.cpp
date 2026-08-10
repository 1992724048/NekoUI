// 2026-08-10

#include "ButtonInput.hpp"

#include <utility>

#include "../../Device/Mouse.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    ButtonInput::ButtonInput(neko::widget::Widget& owner, const behavior::GeometryState& geometry, const engine::Context& /*context*/, OnClick onClick) :
        InputBehavior{owner},
        geometry_(geometry),
        onClick_(std::move(onClick)) {}

    auto ButtonInput::input(engine::Context& context, const platform::Event& event) -> void {
        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            const auto mouse = context.mouse.lock();
            if (onClick_ && mouse && mouse->is_inside(geometry_.bounds)) {
                onClick_();
            }
        }
    }
} // namespace neko::behavior
