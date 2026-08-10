// 2026-08-10 10:19:45

#pragma once
#include "../../Behavior/InputBehavior.hpp"
#include "../../Behavior/GeometryState.hpp"

#include <functional>
#include <utility>

namespace neko::behavior {
    class ButtonInput final : public InputBehavior {
    public:
        using OnClick = std::function<void()>;

        ButtonInput(widget::Widget& owner, const GeometryState& geometry, const engine::Context& context, OnClick on_click);
        auto input(engine::Context& context, const platform::Event& event) -> void override;

        auto set_on_click(OnClick on_click) -> void {
            on_click_ = std::move(on_click);
        }
    private:
        const GeometryState& geometry_;
        OnClick on_click_;
    };
}
