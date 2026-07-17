#include "Center.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Center::Center(engine::Context&) {}

    auto Center::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        if (style_.background_color.value != 0) {
            backend.draw_rect_fill(bounds, style_.background_color);
        }

        if (auto& children = get_children(); children.is_widget()) {
            if (const auto& child = children.as_widget()) {
                const auto cw = child->get_bounds().z - child->get_bounds().x;
                const auto ch = child->get_bounds().w - child->get_bounds().y;
                const auto cx = bounds.x + (bounds.z - bounds.x - cw) / 2;
                const auto cy = bounds.y + (bounds.w - bounds.y - ch) / 2;
                child->set_bounds({{cx, cy, {cx + cw}, {cy + ch}}});
                child->draw({{{cx, cy, {cx + cw}, {cy + ch}}}}, context, backend);
            }
        }

        return Rect{.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }

    auto Center::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }

    auto Center::style(const CenterStyle& s) -> Center& {
        style_ = s;
        return *this;
    }
} // namespace neko::widget
