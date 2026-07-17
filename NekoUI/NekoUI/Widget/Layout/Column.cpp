#include "Column.hpp"

#include <limits>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Column::Column(engine::Context&) {}

    auto Column::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
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
