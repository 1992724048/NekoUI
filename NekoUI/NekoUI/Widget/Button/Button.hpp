#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>

#include "../../Component/Animation.hpp"
#include "../../Style/CSS.hpp"

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(engine::Context& /*unused*/, std::string text = {}, std::function<void()> on_click = {});

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto on_click(std::function<void()> cb) -> Button&;
        auto text(std::string t) -> Button&;

        auto background(style::Background bg) -> Button& {
            background_ = bg;
            return *this;
        }

        auto widget_size(style::Size sz) -> Button& {
            size_ = sz;
            return *this;
        }

        auto border(style::Border bd) -> Button& {
            border_ = bd;
            return *this;
        }

        auto font_size(float fs) -> Button& {
            font_size_ = fs;
            return *this;
        }

        auto text_color(Color tc) -> Button& {
            text_color_ = tc;
            return *this;
        }
    private:
        std::string text_;
        std::function<void()> on_click_;
        style::Background background_;
        style::Size size_{{100, 40}};
        style::Border border_;
        float font_size_ = 16.0f;
        Color text_color_{0xFFFFFFFF};
    };
} // namespace neko::widget
