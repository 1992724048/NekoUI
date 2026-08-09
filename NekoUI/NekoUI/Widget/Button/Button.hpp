#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"
#include "../../Component/Animation.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Button final : public Widget, public style::BackgroundStyle, public style::SizeStyle, public style::BorderStyle, public style::TextStyle {
    public:
        explicit Button(const engine::Context& context, std::string text = {}, std::function<void()> on_click = {});

        auto layout(Vec4I rect, engine::Context& context) -> void override;
        auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto on_click(std::function<void()> cb) -> Button&;
    private:
        std::string text_;
        std::function<void()> on_click_;
        bool hover_{false};
        component::Animation<float> scale_{1.0F, 200};
    };
} // namespace neko::widget
