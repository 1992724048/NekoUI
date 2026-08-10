// 2026-08-10

#pragma once
#include "../../Behavior/LayoutBehavior.hpp"
#include "../../Behavior/GeometryState.hpp"

#include "TextFieldStyle.hpp"

namespace neko::behavior {
    class TextFieldLayout final : public LayoutBehavior {
    public:
        TextFieldLayout(neko::widget::Widget& owner, behavior::GeometryState& geometry, const style::TextFieldStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        behavior::GeometryState& geometry_;
        const style::TextFieldStyle& style_;
    };
} // namespace neko::behavior
