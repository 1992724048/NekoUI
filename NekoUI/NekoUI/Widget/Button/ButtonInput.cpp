// 2026-08-10

#include "ButtonInput.hpp"

#include <cstdio>
#include <utility>

#include "../../Device/Mouse.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    ButtonInput::ButtonInput(neko::widget::Widget& owner, const behavior::GeometryState& geometry, InteractionState& interaction, const engine::Context& /*context*/, OnClick onClick) :
        InputBehavior{owner},
        geometry_(geometry),
        interaction_(interaction),
        onClick_(std::move(onClick)) {}

    auto ButtonInput::input(engine::Context& context, const platform::Event& event) -> void {
        if (const auto* move = std::get_if<device::MouseMoveEvent>(&event); move != nullptr) {
            const auto mouse = context.mouse.lock();
            if (!mouse) {
                return;
            }
            const auto inside = mouse->is_inside(geometry_.bounds);
            std::printf("[Input] widget=%s pos=(%d,%d) bounds=(%d,%d,%d,%d) inside=%d hovered=%d\n",
                        owner_.path().c_str(), move->x, move->y, geometry_.bounds.x, geometry_.bounds.y, geometry_.bounds.z, geometry_.bounds.w,
                        inside, interaction_.hovered.load(std::memory_order_relaxed));
            if (inside != interaction_.hovered.load(std::memory_order_relaxed)) {
                interaction_.hovered.store(inside, std::memory_order_relaxed);
                if (context.mark_dirty) {
                    context.mark_dirty();
                }
            }
            return;
        }

        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            const auto mouse = context.mouse.lock();
            if (onClick_ && mouse && mouse->is_inside(geometry_.bounds)) {
                onClick_();
            }
        }
    }
} // namespace neko::behavior
