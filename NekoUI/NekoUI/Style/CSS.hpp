#pragma once
#include <limits>

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

    struct BackgroundStyle {
        Background background_{Color{0}};
    };

    struct SizeStyle {
        Size size_{.size = {.x = std::numeric_limits<float>::max(), .y = std::numeric_limits<float>::max()}, .margin = {.x = 0.0F, .y = 0.0F}, .padding = {.x = 0.0F, .y = 0.0F}};
    };

    struct BorderStyle {
        Border border_{.size = 0.0F, .color = Color{0}};
    };

    struct TextStyle {
        Color text_color_{0xFFFFFFFF};
        float font_size_ = 16.0F;
    };
} // namespace neko::style
