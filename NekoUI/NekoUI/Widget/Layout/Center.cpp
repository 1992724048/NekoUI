#include "Center.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Engine/WidgetVisitor.hpp"

namespace neko::widget {
    Center::Center(engine::Context&) {}

    auto Center::layout(Vec4I available, engine::Context& context) -> void {
        set_bounds(available);

        engine::visit_children(*this,
                               [&](const std::shared_ptr<widget::Widget>& child) -> void {
                                   // First let child calculate its natural size
                                   child->layout(available, context);
                                   const auto& cb = child->get_bounds();
                                   const auto cw = cb.z - cb.x;
                                   const auto ch = cb.w - cb.y;
                                   const auto cx = available.x + (available.z - available.x - cw) / 2;
                                   const auto cy = available.y + (available.w - available.y - ch) / 2;
                                   child->set_bounds({{cx, cy, {cx + cw}, {cy + ch}}});
                               });
    }

    auto Center::draw(Vec4I /*rect*/, engine::Context& context, backend::Backend& backend) -> Rect {
        auto bg = background_.color.value != 0 ? background_ : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }

    auto Center::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
