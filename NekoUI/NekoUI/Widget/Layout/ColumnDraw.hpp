// 2026-08-10

#pragma once
#include "../../Behavior/DrawBehavior.hpp"

#include "../../Style/CSS.hpp"

namespace neko::behavior {
    class ColumnDraw final : public DrawBehavior {
    public:
        ColumnDraw(neko::widget::Widget& owner, const style::ColumnStyle& style);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const style::ColumnStyle& style_;
    };
}
