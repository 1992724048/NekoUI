// 2026-08-10

#pragma once
#include "../../../Behavior/LayoutBehavior.hpp"
#include "../../../Behavior/GeometryState.hpp"

namespace neko::behavior {
    class CenterLayout final : public LayoutBehavior {
    public:
        explicit CenterLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry) : LayoutBehavior{owner}, geometry_{geometry} {}
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        behavior::GeometryState& geometry_;
    };
}
