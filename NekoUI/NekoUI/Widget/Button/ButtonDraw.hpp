// 2026-08-10 10:19:14

#pragma once
#include "../../Behavior/DrawBehavior.hpp"
#include "../../Behavior/GeometryState.hpp"
#include "../../Behavior/InteractionState.hpp"

#include "../../Component/Animation.hpp"
#include "ButtonStyle.hpp"

#include <string>

namespace neko::behavior {
    class ButtonDraw final : public DrawBehavior {
    public:
        ButtonDraw(widget::Widget& owner, const GeometryState& geometry, const InteractionState& interaction, const style::ButtonStyle& style, const engine::Context& context, std::string text);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const GeometryState& geometry_;
        const InteractionState& interaction_;
        const style::ButtonStyle& style_;
        std::string text_;
        component::Animation<float> scale_{1.0F, 200};
        bool prev_hovered_{false};
    };
}
