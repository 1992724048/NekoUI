#include "Row.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Row::Row(engine::Context&) { horizontal_ = true; }

    auto Row::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        auto effective = rect;
        if (const auto use_parent = style_.size.x == std::numeric_limits<float>::max() || style_.size.y == std::numeric_limits<float>::max(); !use_parent) {
            effective.z = effective.x + static_cast<int>(style_.size.x);
            effective.w = effective.y + static_cast<int>(style_.size.y);
        }

        if (style_.background_color.value != 0) {
            backend.draw_rect_fill(effective, style_.background_color);
        }

        return Rect{.x = effective.x, .y = effective.y, .width = effective.z - effective.x, .height = effective.w - effective.y};
    }

    auto Row::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
