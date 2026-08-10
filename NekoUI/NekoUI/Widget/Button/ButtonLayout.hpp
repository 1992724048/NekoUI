// 2026-08-10

#pragma once
#include "../../Behavior/LayoutBehavior.hpp"

#include "ButtonStyle.hpp"

namespace neko::behavior {
    class ButtonLayout final : public LayoutBehavior {
    public:
        ButtonLayout(neko::widget::Widget& owner, const style::ButtonStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        const style::ButtonStyle& style_;
    };
}
