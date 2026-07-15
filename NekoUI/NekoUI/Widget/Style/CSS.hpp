#pragma once
#include "../../Type.hpp"

namespace neko::css {
    using namespace neko::type;

    struct Background {
        type::Color color;
    };

    struct Size {
        Vec2 size;
        Vec2 margin;
        Vec2 padding;
    };

    struct Border {
        float size;
        type::Color color;
    };
} // namespace neko::css
