// 2026-08-10

#pragma once
#include "../../Behavior/GeometryState.hpp"
#include "../../Behavior/InputBehavior.hpp"
#include "../../Behavior/TextFieldState.hpp"

#include "TextFieldStyle.hpp"

namespace neko::behavior {
    class TextFieldInput final : public InputBehavior {
    public:
        TextFieldInput(neko::widget::Widget& owner, behavior::TextFieldState& state, const behavior::GeometryState& geometry, const style::TextFieldStyle& style, const engine::Context& context);
        auto input(engine::Context& context, const platform::Event& event) -> void override;
    private:
        auto handle_char(engine::Context& context, const wchar_t ch) -> void;
        auto handle_key(engine::Context& context, const device::KeyEvent& key) -> void;
        auto handle_ime(engine::Context& context, const platform::ImeCompositionEvent& ime) -> void;
        behavior::TextFieldState& state_;
        const behavior::GeometryState& geometry_;
        const style::TextFieldStyle& style_;
    };
} // namespace neko::behavior
