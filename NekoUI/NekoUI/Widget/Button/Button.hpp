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
    class Button final : public Widget {
    public:
        using OnClick = ButtonInput::OnClick;

        Button(const engine::Context& context, std::string text = "", OnClick onClick = nullptr)
            : input_{add_behavior<ButtonInput>(context, std::move(onClick))} {
            add_behavior<ButtonLayout>(style_);
            add_behavior<ButtonDraw>(style_, context, std::move(text));
            add_behavior<ButtonHitTest>();
        }

        auto on_click(OnClick callback) -> Button& {
            input_.set_on_click(std::move(callback));
            return *this;
        }

        auto style() -> style::ButtonStyle& {
            return style_;
        }
    private:
        style::ButtonStyle style_{};
        ButtonInput& input_;
    };
} // namespace neko::widget
