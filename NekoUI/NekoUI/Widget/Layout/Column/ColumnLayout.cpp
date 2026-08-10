// 2026-08-10

#include "ColumnLayout.hpp"

#include <limits>

#include "../../../Engine/Tree/WidgetVisitor.hpp"
#include "ColumnStyle.hpp"
#include "../../Widget.hpp"

namespace neko::behavior {
    ColumnLayout::ColumnLayout(neko::widget::Widget& owner, const style::ColumnStyle& style) :
        LayoutBehavior{owner},
        style_{style} {}

    auto ColumnLayout::layout(const Vec4I available, engine::Context& context) -> void {
        auto effective = available;
        const auto use_parent = style_.size.value.x == std::numeric_limits<float>::max() || style_.size.value.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(style_.size.value.x);
            effective.w = effective.y + static_cast<int>(style_.size.value.y);
        }
        owner_.set_bounds(effective);

        auto y_offset = effective.y;
        engine::visit_children(owner_,
                               [&](const std::shared_ptr<neko::widget::Widget>& child) -> void {
                                   child->layout({.x = effective.x, .y = y_offset, .z = effective.z, .w = effective.w}, context);
                                   const auto& cb = child->get_bounds();
                                   y_offset += cb.w - cb.y;
                               });
    }
} // namespace neko::behavior
