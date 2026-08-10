// 2026-08-10

#pragma once
#include "../../../Behavior/DrawBehavior.hpp"
#include "../../../Behavior/GeometryState.hpp"

#include "RowStyle.hpp"

namespace neko::behavior {
    class RowDraw final : public DrawBehavior {
    public:
        RowDraw(neko::widget::Widget& owner, const behavior::GeometryState& geometry, const style::RowStyle& style);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const behavior::GeometryState& geometry_;
        const style::RowStyle& style_;
    };
}
