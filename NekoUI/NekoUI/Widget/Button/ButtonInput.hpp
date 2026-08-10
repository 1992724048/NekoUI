// 2026-08-10

#pragma once
#include "../../Behavior/InputBehavior.hpp"

#include <functional>
#include <utility>

namespace neko::behavior {
    class ButtonInput final : public InputBehavior {
    public:
        using OnClick = std::function<void()>;

        ButtonInput(neko::widget::Widget& owner, const engine::Context& context, OnClick onClick);
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        auto set_on_click(OnClick onClick) -> void { onClick_ = std::move(onClick); }
    private:
        OnClick onClick_;
    };
}
