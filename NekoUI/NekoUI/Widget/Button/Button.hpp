#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>
#include <string_view>

#include "../../Component/Animation.hpp"

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(Widget* parent, Vec4I bounds = {}, std::string_view label = "");
        explicit Button(engine::Context& context, Vec4I bounds = {}, std::string_view label = "");

        auto set_text(std::string_view text) -> void;
        [[nodiscard]] auto text() const -> std::string;
        auto set_color(Color color) -> void;
        [[nodiscard]] auto color() const -> Color;
        auto set_on_click(std::function<void()> callback) -> void;
        auto animate_color(Color target, int duration_ms = 200) -> void;
    protected:
        auto draw_self(engine::Context& context, backend::Backend& backend) -> void override;
    private:
        std::string text_{};
        Color color_{0xFF4A90D9};
        std::function<void()> on_click_{};
        component::Animation<int, component::ease::quad::in_out> color_anim_{0, 200};
    };
} // namespace neko::widget
