// 2026-08-10

#pragma once
#include "../../../Behavior/LayoutBehavior.hpp"

namespace neko::behavior {
    class CenterLayout final : public LayoutBehavior {
    public:
        explicit CenterLayout(neko::widget::Widget& owner) : LayoutBehavior{owner} {}
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    };
}
