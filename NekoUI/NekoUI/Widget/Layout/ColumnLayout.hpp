// 2026-08-10

#pragma once
#include "../Behavior/LayoutBehavior.hpp"

namespace neko::widget {
    class ColumnLayout final : public LayoutBehavior {
    public:
        explicit ColumnLayout(Widget& owner) : LayoutBehavior{owner} {}
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    };
}
