#pragma once
#include "../Widget.hpp"
#include "../../Style/Stylable.hpp"

#include <functional>
#include <string>

#include "../../Component/Animation.hpp"

namespace neko::widget {
    class Button final : public Widget, public Stylable<Button> {
    public:
        explicit Button(engine::Context& /*unused*/, std::string text = {}, std::function<void()> on_click = {});

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto on_click(std::function<void()> cb) -> Button&;
    private:
        std::string text_;
        std::function<void()> on_click_;
    };
} // namespace neko::widget
