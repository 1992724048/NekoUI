// 2026-08-10

#include "RowLayout.hpp"

#include <limits>

#include "../../Engine/WidgetVisitor.hpp"
#include "../Widget.hpp"
#include "Row.hpp"

namespace neko::widget {
    auto RowLayout::layout(const Vec4I available, engine::Context& context) -> void {
        auto& row = static_cast<Row&>(owner_);
        auto effective = available;
        const auto use_parent = row.size_.size.x == std::numeric_limits<float>::max() || row.size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(row.size_.size.x);
            effective.w = effective.y + static_cast<int>(row.size_.size.y);
        }
        row.set_bounds(effective);

        auto x_offset = effective.x;
        engine::visit_children(row,
                               [&](const std::shared_ptr<Widget>& child) -> void {
                                   child->layout({.x = x_offset, .y = effective.y, .z = effective.z, .w = effective.w}, context);
                                   const auto& cb = child->get_bounds();
                                   x_offset += cb.z - cb.x;
                               });
    }
} // namespace neko::widget
