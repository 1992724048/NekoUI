// 2026-08-10

#include "CenterLayout.hpp"

#include "../../Engine/WidgetVisitor.hpp"
#include "../Widget.hpp"
#include "Center.hpp"

namespace neko::widget {
    auto CenterLayout::layout(const Vec4I available, engine::Context& context) -> void {
        auto& center = static_cast<Center&>(owner_);
        center.set_bounds(available);

        engine::visit_children(center,
                               [&](const std::shared_ptr<Widget>& child) -> void {
                                   // First let child calculate its natural size
                                   child->layout(available, context);
                                   const auto& cb = child->get_bounds();
                                   const auto cw = cb.z - cb.x;
                                   const auto ch = cb.w - cb.y;
                                   const auto cx = available.x + (available.z - available.x - cw) / 2;
                                   const auto cy = available.y + (available.w - available.y - ch) / 2;
                                   child->set_bounds({{cx, cy, {cx + cw}, {cy + ch}}});
                               });
    }
} // namespace neko::widget
