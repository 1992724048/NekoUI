// 2026-08-10

#pragma once
#include "../../Behavior/DrawBehavior.hpp"
#include "../../Behavior/GeometryState.hpp"
#include "../../Behavior/InteractionState.hpp"
#include "../../Behavior/TextFieldState.hpp"

#include "TextFieldStyle.hpp"

#include <string>

namespace neko::behavior {
    class TextFieldDraw final : public DrawBehavior {
    public:
        TextFieldDraw(neko::widget::Widget& owner, const behavior::GeometryState& geometry, const InteractionState& interaction, const TextFieldState& state, const style::TextFieldStyle& style, const engine::Context& context);
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
    private:
        const behavior::GeometryState& geometry_;
        const InteractionState& interaction_;
        const TextFieldState& state_;
        const style::TextFieldStyle& style_;
    };
} // namespace neko::behavior
