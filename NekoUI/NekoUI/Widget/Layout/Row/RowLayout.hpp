// 2026-08-10

#pragma once
#include "../../../Behavior/LayoutBehavior.hpp"
#include "../../../Behavior/GeometryState.hpp"

#include "RowStyle.hpp"

namespace neko::behavior {
    class RowLayout final : public LayoutBehavior {
    public:
        RowLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry, const style::RowStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        behavior::GeometryState& geometry_;
        const style::RowStyle& style_;
    };
}
