// 2026-08-10

#pragma once
#include "../Widget.hpp"

#include "TextFieldDraw.hpp"
#include "TextFieldHitTest.hpp"
#include "TextFieldInput.hpp"
#include "TextFieldLayout.hpp"
#include "TextFieldStyle.hpp"

#include <string>

namespace neko::widget {
    using behavior::TextFieldDraw;
    using behavior::TextFieldHitTest;
    using behavior::TextFieldInput;
    using behavior::TextFieldLayout;

    class TextField final : public Widget {
    public:
        explicit TextField(const engine::Context& context)
            : input_{add_behavior<behavior::TextFieldInput>(state_, geometry_, style_, context)} {
            set_geometry(geometry_);
            set_interaction(interaction_);
            add_behavior<behavior::TextFieldLayout>(geometry_, style_);
            add_behavior<behavior::TextFieldDraw>(geometry_, interaction_, state_, style_, context);
            add_behavior<behavior::TextFieldHitTest>(geometry_);
        }

        auto style() -> style::TextFieldStyle& {
            return style_;
        }

        [[nodiscard]] auto text() const -> const std::string& {
            return state_.text;
        }
    private:
        style::TextFieldStyle style_{};
        behavior::TextFieldState state_{};
        behavior::InteractionState interaction_{};
        behavior::GeometryState geometry_{};
        behavior::TextFieldInput& input_;
    };
} // namespace neko::widget
