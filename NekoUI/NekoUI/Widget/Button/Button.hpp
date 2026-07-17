#pragma once
#include "../Widget.hpp"
#include "../Stylable.hpp"

#include <functional>
#include <string>

#include "../../Component/Animation.hpp"

namespace neko::widget {
    class Button final : public Widget, public Stylable<Button> {
    public:
        explicit Button(engine::Context&, std::string text = {}, std::function<void()> on_click = {});

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto on_click(std::function<void()> cb) -> Button&;
        auto text(std::string t) -> Button&;

        auto font_size(float fs) -> Button& { font_size_ = fs; return *this; }
        auto text_color(Color tc) -> Button& { text_color_ = tc; return *this; }

    private:
        std::string text_;
        std::function<void()> on_click_;
        float font_size_ = 16.0f;
        Color text_color_{0xFFFFFFFF};
    };
} // namespace neko::widget
