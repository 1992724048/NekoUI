// 2026-08-10 10:19:21

#include "ButtonInput.hpp"

#include <utility>

#include "../../Device/Mouse.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    ButtonInput::ButtonInput(widget::Widget& owner, const GeometryState& geometry, const engine::Context& /*context*/, OnClick on_click) :
        InputBehavior{owner},
        geometry_(geometry),
        on_click_(std::move(on_click)) {}

    auto ButtonInput::input(engine::Context& context, const platform::Event& event) -> void {
        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            const auto mouse = context.mouse.lock();
            if (on_click_ && mouse && mouse->is_inside(geometry_.bounds)) {
                on_click_();
            }
        }
    }
} // namespace neko::behavior
