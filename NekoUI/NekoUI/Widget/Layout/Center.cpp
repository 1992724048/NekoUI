#include "Center.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Center::Center(engine::Context&) {}

    auto Center::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        if (style_.background_color.value != 0) {
            backend.draw_rect_fill(bounds, style_.background_color);
        }

        // 居中放置子控件
        auto& children = get_children();
        if (children.is_widget()) {
            if (const auto& child = children.as_widget()) {
                const auto cw = child->bounds.z - child->bounds.x;
                const auto ch = child->bounds.w - child->bounds.y;
                const auto cx = bounds.x + (bounds.z - bounds.x - cw) / 2;
                const auto cy = bounds.y + (bounds.w - bounds.y - ch) / 2;
                child->bounds = type::Vec4I{cx, cy, cx + cw, cy + ch};
                child->draw(type::Vec4I{cx, cy, cx + cw, cy + ch}, context, backend);
            }
        }

        return Rect{.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }

    auto Center::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
