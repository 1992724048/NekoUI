#pragma once
#include "../Type.hpp"

namespace neko::style {
    using namespace neko::type;

    struct Background {
        Color color;
    };

    struct Size {
        Vec2 size;
        Vec2 margin;
        Vec2 padding;
    };

    struct Border {
        float size;
        Color color;
    };
} // namespace neko::css
