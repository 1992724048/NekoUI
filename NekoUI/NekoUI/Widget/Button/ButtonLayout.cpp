// 2026-08-10

#include "ButtonLayout.hpp"

#include <limits>

#include "Button.hpp"

namespace neko::widget {
    auto ButtonLayout::layout(const Vec4I available, engine::Context& /*context*/) -> void {
        auto& button = static_cast<Button&>(owner_);
        auto effective = available;
        const auto use_parent = button.size_.size.x == std::numeric_limits<float>::max() || button.size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(button.size_.size.x);
            effective.w = effective.y + static_cast<int>(button.size_.size.y);
        }
        button.set_bounds(effective);
    }
} // namespace neko::widget
