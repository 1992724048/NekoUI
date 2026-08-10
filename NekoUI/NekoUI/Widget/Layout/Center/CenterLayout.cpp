// 2026-08-10

#include "CenterLayout.hpp"

#include "../../../Engine/Tree/WidgetVisitor.hpp"
#include "../../Widget.hpp"

namespace neko::behavior {
    auto CenterLayout::layout(const Vec4I available, engine::Context& context) -> void {
        geometry_.bounds = available;

        engine::visit_children(owner_,
                               [&](const std::shared_ptr<neko::widget::Widget>& child) -> void {
                                   child->layout(available, context);
                                   const auto& cb = child->get_bounds();
                                   const auto cw = cb.z - cb.x;
                                   const auto ch = cb.w - cb.y;
                                   const auto cx = available.x + (available.z - available.x - cw) / 2;
                                   const auto cy = available.y + (available.w - available.y - ch) / 2;
                                   child->geometry().bounds = {.x = cx, .y = cy, .z = cx + cw, .w = cy + ch};
                               });
    }
} // namespace neko::behavior
