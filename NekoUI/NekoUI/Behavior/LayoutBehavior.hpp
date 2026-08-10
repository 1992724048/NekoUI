// 2026-08-10

#pragma once
#include "Behavior.hpp"

#include "../Type.hpp"

namespace neko::engine {
    struct Context;
}

namespace neko::behavior {
    using namespace neko::type;

    class LayoutBehavior : public Behavior {
    public:
        explicit LayoutBehavior(neko::widget::Widget& owner) : Behavior{owner} {}
        ~LayoutBehavior() override = default;
        virtual auto layout(Vec4I rect, engine::Context& context) -> void = 0;
    };
}
