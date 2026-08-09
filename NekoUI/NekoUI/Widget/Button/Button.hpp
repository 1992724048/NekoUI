// 2026-08-10

#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"

#include "ButtonDraw.hpp"
#include "ButtonHitTest.hpp"
#include "ButtonInput.hpp"
#include "ButtonLayout.hpp"

#include <string>
#include <utility>

namespace neko::widget {
    class Button final : public Widget, public style::BackgroundStyle, public style::SizeStyle, public style::BorderStyle, public style::TextStyle {
    public:
        using OnClick = ButtonInput::OnClick;

        explicit Button(const engine::Context& context, std::string text = "", OnClick onClick = nullptr)
            : input_{add_behavior<ButtonInput>(context, std::move(onClick))} {
            add_behavior<ButtonLayout>();
            add_behavior<ButtonDraw>(context, std::move(text));
            add_behavior<ButtonHitTest>();
        }

        auto on_click(OnClick callback) -> Button& {
            input_.set_on_click(std::move(callback));
            return *this;
        }
    private:
        ButtonInput& input_;
    };
} // namespace neko::widget
