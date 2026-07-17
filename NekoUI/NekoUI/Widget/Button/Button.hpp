#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>

#include "../../Component/Animation.hpp"

namespace neko::widget {

/// Button 样式
struct ButtonStyle {
    type::Color background_color{0xFF1E1E1E};
    type::Color text_color{0xFFFFFFFF};
    float font_size{16.0f};
    type::Vec2 size{100, 40};
    float border_size{0.0f};
    type::Color border_color{0x00000000};
};

class Button final : public Widget {
public:
    explicit Button(engine::Context&, std::string text = {}, std::function<void()> on_click = {});

    auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void override;
    auto build(engine::Context& context) -> void override;
    auto event(engine::Context& context) -> void override;
    auto input(engine::Context& context, const platform::Event& event) -> void override;
    [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

    /// 链式 setter：设置样式
    auto style(ButtonStyle s) -> Button& {
        style_ = s;
        return *this;
    }

    /// 链式 setter：设置点击回调
    auto on_click(std::function<void()> cb) -> Button& {
        on_click_ = std::move(cb);
        return *this;
    }

    /// 链式 setter：设置按钮文字
    auto text(std::string t) -> Button& {
        text_ = std::move(t);
        return *this;
    }

private:
    std::string text_;
    std::function<void()> on_click_;
    ButtonStyle style_{};
};

} // namespace neko::widget
