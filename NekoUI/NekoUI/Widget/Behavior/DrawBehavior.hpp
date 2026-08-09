// 2026-08-10

#pragma once
#include "Behavior.hpp"

#include "../../Type.hpp"

namespace neko::backend {
    class DirectX11;
}

namespace neko::engine {
    class Context;
}

namespace neko::widget {
    using namespace neko::type;

    class DrawBehavior : public Behavior {
    public:
        explicit DrawBehavior(Widget& owner) : Behavior{owner} {}
        virtual ~DrawBehavior() = default;
        virtual auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect = 0;
    };
}
