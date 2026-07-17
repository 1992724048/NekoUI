#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>

#include "../../Component/Animation.hpp"

namespace neko::widget {
    struct ButtonStyle {
        Color background_color{0xFF1E1E1E};
        Color text_color{0xFFFFFFFF};
        float font_size{16.0F};
        Size size{.x = std::numeric_limits<int>::max(), .y = std::numeric_limits<int>::max()};
        float border_size{0.0F};
        Color border_color{};
    };

    class Button final : public Widget {
    public:
        explicit Button(engine::Context& /*unused*/, std::string text = {}, std::function<void()> on_click = {});

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto style(const ButtonStyle& s) -> Button&;

        auto on_click(std::function<void()> cb) -> Button&;

        auto text(std::string t) -> Button&;
    private:
        std::string text_;
        std::function<void()> on_click_;
        ButtonStyle style_{};
    };

} // namespace neko::widget
