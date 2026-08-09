// 2026-08-10

#pragma once
#include "../Behavior/DrawBehavior.hpp"

#include "../../Component/Animation.hpp"
#include "../../Style/CSS.hpp"

#include <string>

namespace neko::widget {
    class ButtonDraw final : public DrawBehavior {
    public:
        ButtonDraw(Widget& owner, const style::ButtonStyle& style, const engine::Context& context, std::string text);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const style::ButtonStyle& style_;
        std::string text_;
        component::Animation<float> scale_{1.0F, 200};
        bool prev_hovered_{false};
    };
}
