// 2026-08-10

#pragma once
#include "Behavior.hpp"

#include "../Platform/Event.hpp"

namespace neko::engine {
    struct Context;
}

namespace neko::behavior {
    class InputBehavior : public Behavior {
    public:
        explicit InputBehavior(neko::widget::Widget& owner) : Behavior{owner} {}
        ~InputBehavior() override = default;
        virtual auto input(engine::Context& context, const platform::Event& event) -> void = 0;
    };
}
