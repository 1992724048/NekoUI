// 2026-08-10

#include "RowLayout.hpp"

#include <limits>

#include "../../../Engine/Tree/WidgetVisitor.hpp"
#include "RowStyle.hpp"
#include "../../Widget.hpp"

namespace neko::behavior {
    RowLayout::RowLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry, const style::RowStyle& style) :
        LayoutBehavior{owner},
        geometry_{geometry},
        style_{style} {}

    auto RowLayout::layout(const Vec4I available, engine::Context& context) -> void {
        auto effective = available;
        const auto use_parent = style_.size.value.x == std::numeric_limits<float>::max() || style_.size.value.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(style_.size.value.x);
            effective.w = effective.y + static_cast<int>(style_.size.value.y);
        }
        geometry_.bounds = effective;

        auto x_offset = effective.x;
        engine::visit_children(owner_,
                               [&](const std::shared_ptr<neko::widget::Widget>& child) -> void {
                                   child->layout({.x = x_offset, .y = effective.y, .z = effective.z, .w = effective.w}, context);
                                   const auto& cb = child->get_bounds();
                                   x_offset += cb.z - cb.x;
                               });
    }
} // namespace neko::behavior
