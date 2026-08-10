// 2026-08-10 10:16:49

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
        explicit TextField(const engine::Context& context) :
            input_{add_behavior<TextFieldInput>(state_, geometry_, style_, context)} {
            set_geometry(geometry_);
            set_interaction(interaction_);
            add_behavior<TextFieldLayout>(geometry_, style_);
            add_behavior<TextFieldDraw>(geometry_, interaction_, state_, style_, context);
            add_behavior<TextFieldHitTest>(geometry_);
        }

        // 绑定外部字符串——输入实时写入 bound_text（仅读取，不拥有生命周期）
        TextField(const engine::Context& context, std::string& bound_text) : TextField(context) {
            input_.set_bound_text(&bound_text);
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
        TextFieldInput& input_;
    };
} // namespace neko::widget
