// 2026-08-10

#include "ButtonLayout.hpp"

#include <limits>

#include "ButtonStyle.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    ButtonLayout::ButtonLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry, const style::ButtonStyle& style) :
        LayoutBehavior{owner},
        geometry_{geometry},
        style_{style} {}

    auto ButtonLayout::layout(const Vec4I available, engine::Context& /*context*/) -> void {
        auto effective = available;
        const auto use_parent = style_.size.value.x == std::numeric_limits<float>::max() || style_.size.value.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(style_.size.value.x);
            effective.w = effective.y + static_cast<int>(style_.size.value.y);
        }
        geometry_.bounds = effective;
    }
} // namespace neko::behavior
