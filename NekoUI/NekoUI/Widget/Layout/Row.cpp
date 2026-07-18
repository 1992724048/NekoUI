#include "Row.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Row::Row(engine::Context&) {
        horizontal_ = true;
    }

    auto Row::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        // 使用 ColorScheme 默认值
        auto bg = background_.color.value != 0 ? background_ : style::Background{context.scheme.surface};

        auto effective = rect;
        const auto use_parent = size_.size.x == std::numeric_limits<float>::max() || size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(size_.size.x);
            effective.w = effective.y + static_cast<int>(size_.size.y);
        }

        if (bg.color.value != 0) {
            backend.draw_rect_fill(effective, bg.color);
        }

        return Rect{.x = effective.x, .y = effective.y, .width = effective.z - effective.x, .height = effective.w - effective.y};
    }

    auto Row::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
