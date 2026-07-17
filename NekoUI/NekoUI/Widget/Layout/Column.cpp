#include "Column.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Column::Column(engine::Context&) {}

    auto Column::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        // 当 size 为 max 时使用父容器传入的 rect，否则用自身 bounds
        const auto use_parent_rect = style_.size.x == std::numeric_limits<float>::max() || style_.size.y == std::numeric_limits<float>::max();
        const auto effective = use_parent_rect ? rect : bounds;

        if (style_.background_color.value != 0) {
            backend.draw_rect_fill(effective, style_.background_color);
        }

        return Rect{.x = effective.x, .y = effective.y, .width = effective.z - effective.x, .height = effective.w - effective.y};
    }

    auto Column::build(engine::Context& context) -> void {}

    auto Column::event(engine::Context& context) -> void {}

    auto Column::input(engine::Context& context, const platform::Event& event) -> void {}

    auto Column::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }

    auto Column::style(const ColumnStyle& s) -> Column& {
        style_ = s;
        return *this;
    }
} // namespace neko::widget
