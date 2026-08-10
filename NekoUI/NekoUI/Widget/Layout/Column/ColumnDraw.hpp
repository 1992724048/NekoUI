// 2026-08-10

#pragma once
#include "../../../Behavior/DrawBehavior.hpp"
#include "../../../Behavior/GeometryState.hpp"

#include "ColumnStyle.hpp"

namespace neko::behavior {
    class ColumnDraw final : public DrawBehavior {
    public:
        ColumnDraw(neko::widget::Widget& owner, const behavior::GeometryState& geometry, const style::ColumnStyle& style);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const behavior::GeometryState& geometry_;
        const style::ColumnStyle& style_;
    };
}
