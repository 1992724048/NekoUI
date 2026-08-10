// 2026-08-10

#pragma once
#include "../../Behavior/DrawBehavior.hpp"

#include "../../Style/CSS.hpp"

namespace neko::behavior {
    class CenterDraw final : public DrawBehavior {
    public:
        CenterDraw(neko::widget::Widget& owner, const style::CenterStyle& style);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const style::CenterStyle& style_;
    };
}
