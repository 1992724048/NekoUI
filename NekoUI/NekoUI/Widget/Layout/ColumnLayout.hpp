// 2026-08-10

#pragma once
#include "../Behavior/LayoutBehavior.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {
    class ColumnLayout final : public LayoutBehavior {
    public:
        ColumnLayout(Widget& owner, const style::ColumnStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        const style::ColumnStyle& style_;
    };
}
