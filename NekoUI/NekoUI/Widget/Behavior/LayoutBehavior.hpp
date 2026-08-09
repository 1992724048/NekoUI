// 2026-08-10

#pragma once
#include "Behavior.hpp"

#include "../../Type.hpp"

namespace neko::engine {
    class Context;
}

namespace neko::widget {
    using namespace neko::type;

    class LayoutBehavior : public Behavior {
    public:
        explicit LayoutBehavior(Widget& owner) : Behavior{owner} {}
        virtual ~LayoutBehavior() = default;
        virtual auto layout(Vec4I rect, engine::Context& context) -> void = 0;
    };
}
