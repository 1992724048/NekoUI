// 2026-08-10

#pragma once
#include "../Behavior/DrawBehavior.hpp"

namespace neko::widget {
    class RowDraw final : public DrawBehavior {
    public:
        explicit RowDraw(Widget& owner) : DrawBehavior{owner} {}
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    };
}
