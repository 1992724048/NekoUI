// 2026-08-10

#include "ColumnLayout.hpp"

#include <limits>

#include "../../Engine/WidgetVisitor.hpp"
#include "../Widget.hpp"
#include "Column.hpp"

namespace neko::widget {
    auto ColumnLayout::layout(const Vec4I available, engine::Context& context) -> void {
        auto& column = static_cast<Column&>(owner_);
        auto effective = available;
        const auto use_parent = column.size_.size.x == std::numeric_limits<float>::max() || column.size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(column.size_.size.x);
            effective.w = effective.y + static_cast<int>(column.size_.size.y);
        }
        column.set_bounds(effective);

        auto y_offset = effective.y;
        engine::visit_children(column,
                               [&](const std::shared_ptr<Widget>& child) -> void {
                                   child->layout({.x = effective.x, .y = y_offset, .z = effective.z, .w = effective.w}, context);
                                   const auto& cb = child->get_bounds();
                                   y_offset += cb.w - cb.y;
                               });
    }
} // namespace neko::widget
