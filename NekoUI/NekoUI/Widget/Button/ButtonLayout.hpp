// 2026-08-10

#pragma once
#include "../Behavior/LayoutBehavior.hpp"

namespace neko::widget {
    class ButtonLayout final : public LayoutBehavior {
    public:
        explicit ButtonLayout(Widget& owner) : LayoutBehavior{owner} {}
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    };
}
