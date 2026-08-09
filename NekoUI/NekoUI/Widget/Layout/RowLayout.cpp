// 2026-08-10

#include "RowLayout.hpp"

#include <limits>

#include "../../Engine/WidgetVisitor.hpp"
#include "../../Style/CSS.hpp"
#include "../Widget.hpp"

namespace neko::widget {
    RowLayout::RowLayout(Widget& owner, const style::RowStyle& style) :
        LayoutBehavior{owner},
        style_{style} {}

    auto RowLayout::layout(const Vec4I available, engine::Context& context) -> void {
        auto effective = available;
        const auto use_parent = style_.size.value.x == std::numeric_limits<float>::max() || style_.size.value.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(style_.size.value.x);
            effective.w = effective.y + static_cast<int>(style_.size.value.y);
        }
        owner_.set_bounds(effective);

        auto x_offset = effective.x;
        engine::visit_children(owner_,
                               [&](const std::shared_ptr<Widget>& child) -> void {
                                   child->layout({.x = x_offset, .y = effective.y, .z = effective.z, .w = effective.w}, context);
                                   const auto& cb = child->get_bounds();
                                   x_offset += cb.z - cb.x;
                               });
    }
} // namespace neko::widget
