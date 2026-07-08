#pragma once
#include "../../Type.hpp"

namespace neko::css {
    struct Background {
        type::Color color;
    };

    struct Size {
        glm::vec2 size;
        glm::vec2 margin;
        glm::vec2 padding;
    };

    struct Border {
        float size;
        type::Color color;
    };
} // namespace neko::css
