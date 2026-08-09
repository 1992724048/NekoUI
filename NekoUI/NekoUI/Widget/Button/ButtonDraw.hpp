// 2026-08-10

#pragma once
#include "../Behavior/DrawBehavior.hpp"

#include "../../Component/Animation.hpp"

#include <string>

namespace neko::widget {
    class ButtonDraw final : public DrawBehavior {
    public:
        ButtonDraw(Widget& owner, const engine::Context& context, std::string text);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        std::string text_;
        component::Animation<float> scale_{1.0F, 200};
        bool prev_hovered_{false};
    };
}
