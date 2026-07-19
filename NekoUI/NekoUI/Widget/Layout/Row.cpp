#include "Row.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Engine/WidgetVisitor.hpp"

namespace neko::widget {
    Row::Row(engine::Context&) {}

    auto Row::layout(Vec4I available, engine::Context& context) -> void {
        auto effective = available;
        const auto use_parent = size_.size.x == std::numeric_limits<float>::max() || size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(size_.size.x);
            effective.w = effective.y + static_cast<int>(size_.size.y);
        }
        set_bounds(effective);

        auto x_offset = effective.x;
        engine::visit_children(*this,
                               [&](const std::shared_ptr<widget::Widget>& child) -> void {
                                   child->layout({x_offset, effective.y, effective.z, effective.w}, context);
                                   const auto& cb = child->get_bounds();
                                   x_offset += cb.z - cb.x;
                               });
    }

    auto Row::draw(Vec4I /*rect*/, engine::Context& context, backend::Backend& backend) -> Rect {
        auto bg = background_.color.value != 0 ? background_ : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }

    auto Row::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
