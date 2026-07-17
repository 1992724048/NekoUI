#include "Row.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Row::Row(engine::Context&) { horizontal_ = true; }

    auto Row::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        const auto use_parent = style_.size.x == std::numeric_limits<float>::max() || style_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            bounds.z = bounds.x + static_cast<int>(style_.size.x);
            bounds.w = bounds.y + static_cast<int>(style_.size.y);
        }

        if (style_.background_color.value != 0) {
            backend.draw_rect_fill(bounds, style_.background_color);
        }

        return Rect{.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }

    auto Row::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
