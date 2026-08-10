#pragma once
#include <limits>

#include "../Type.hpp"

namespace neko::style {
    using namespace neko::type;

    struct Background {
        Color color{};
    };

    struct Size {
        Vec2 value{.x = std::numeric_limits<float>::max(), .y = std::numeric_limits<float>::max()};
    };

    struct Border {
        float width{0.0F};
        Color color{};
    };

    struct Text {
        type::Color color{0xFFFFFFFF};
        float font_size{16.0F};
    };
} // namespace neko::style
