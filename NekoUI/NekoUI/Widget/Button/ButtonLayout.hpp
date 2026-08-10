// 2026-08-10

#pragma once
#include "../../Behavior/LayoutBehavior.hpp"
#include "../../Behavior/GeometryState.hpp"

#include "ButtonStyle.hpp"

namespace neko::behavior {
    class ButtonLayout final : public LayoutBehavior {
    public:
        ButtonLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry, const style::ButtonStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        behavior::GeometryState& geometry_;
        const style::ButtonStyle& style_;
    };
}
