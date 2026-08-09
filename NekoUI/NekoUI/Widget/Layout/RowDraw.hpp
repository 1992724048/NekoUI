// 2026-08-10

#pragma once
#include "../Behavior/DrawBehavior.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {
    class RowDraw final : public DrawBehavior {
    public:
        RowDraw(Widget& owner, const style::RowStyle& style);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const style::RowStyle& style_;
    };
}
