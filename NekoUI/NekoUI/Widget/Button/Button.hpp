#pragma once
#include "../StyledWidget.hpp"

#include <functional>
#include <string>
#include <string_view>

#include "../../Component/Animation.hpp"

namespace neko::widget {
    class Button final : public StyledWidget<Button> {
    public:
        explicit Button(std::string text = {}, std::function<void()> on_click = {});

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

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
    };
} // namespace neko::widget
