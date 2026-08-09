#pragma once
#include <limits>

#include "../Type.hpp"

namespace neko::style {
    using namespace neko::type;

    struct Background {
        Color color;
    };

    struct Size {
        Vec2 value{.x = std::numeric_limits<float>::max(), .y = std::numeric_limits<float>::max()};
    };

    struct Border {
        float width{0.0F};
        Color color;
    };

    struct Text {
        type::Color color{0xFFFFFFFF};
        float font_size{16.0F};
    };

    // 控件样式表（组合生成）
    struct ButtonStyle {
        Background background;
        Size size;
        Border border;
        Text text;
    };

    struct ColumnStyle {
        Background background;
        Size size;
    };

    struct RowStyle {
        Background background;
        Size size;
    };

    struct CenterStyle {
        Background background;
    };

    struct BackgroundStyle {
        Background background_{Color{0}};
    };

    struct SizeStyle {
        Size size_{.value = {.x = std::numeric_limits<float>::max(), .y = std::numeric_limits<float>::max()}};
    };

    struct BorderStyle {
        Border border_{.width = 0.0F, .color = Color{0}};
    };

    struct TextStyle {
        Color text_color_{0xFFFFFFFF};
        float font_size_ = 16.0F;
    };
} // namespace neko::style
