// 2026-08-10

#pragma once
#include "../Behavior/LayoutBehavior.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {
    class ButtonLayout final : public LayoutBehavior {
    public:
        ButtonLayout(Widget& owner, const style::ButtonStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        const style::ButtonStyle& style_;
    };
}
