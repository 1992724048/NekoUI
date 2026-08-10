// 2026-08-10 10:18:19

#pragma once
#include "Behavior.hpp"

#include "../Type.hpp"

namespace neko::backend {
    class DirectX11;
}

namespace neko::engine {
    struct Context;
}

namespace neko::behavior {
    using namespace neko::type;

    class DrawBehavior : public Behavior {
    public:
        explicit DrawBehavior(widget::Widget& owner) :
            Behavior{owner} {}

        ~DrawBehavior() override = default;
        virtual auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect = 0;
    };
}
