// 2026-08-10 10:19:01

#pragma once
#include "../Widget.hpp"

#include "ButtonStyle.hpp"

#include "ButtonDraw.hpp"
#include "ButtonHitTest.hpp"
#include "ButtonInput.hpp"
#include "ButtonLayout.hpp"

#include <string>
#include <utility>

namespace neko::widget {
    using behavior::ButtonDraw;
    using behavior::ButtonHitTest;
    using behavior::ButtonInput;
    using behavior::ButtonLayout;

    class Button final : public Widget {
    public:
        using OnClick = ButtonInput::OnClick;

        explicit Button(const engine::Context& context, std::string text = "", OnClick on_click = nullptr) :
            input_{add_behavior<ButtonInput>(geometry_, context, std::move(on_click))} {
            set_geometry(geometry_);
            set_interaction(interaction_);
            add_behavior<ButtonLayout>(geometry_, style_);
            add_behavior<ButtonDraw>(geometry_, interaction_, style_, context, std::move(text));
            add_behavior<ButtonHitTest>(geometry_);
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
        behavior::InteractionState interaction_{};
        behavior::GeometryState geometry_{};
        ButtonInput& input_;
    };
} // namespace neko::widget
