#include "Center.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Center::Center(engine::Context&) {}

    auto Center::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        // 使用 ColorScheme 默认值
        auto bg = background_.color.value != 0 ? background_ : style::Background{context.scheme.surface};

        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
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
} // namespace neko::widget
