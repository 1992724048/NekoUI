// 2026-08-10

#pragma once
#include "../../../Behavior/LayoutBehavior.hpp"
#include "../../../Behavior/GeometryState.hpp"

#include "ColumnStyle.hpp"

namespace neko::behavior {
    class ColumnLayout final : public LayoutBehavior {
    public:
        ColumnLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry, const style::ColumnStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        behavior::GeometryState& geometry_;
        const style::ColumnStyle& style_;
    };
}
