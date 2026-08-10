// 2026-08-10 11:01:28

#pragma once
#include "../../Style/CSS.hpp"

namespace neko::style {
    struct TextFieldStyle {
        Background background;
        Size size;
        Border border;
        Text text{.color = Color{0}};
        Color caret_color{};
        Color caret_color_focus{0xFF0000FF};
        Color comp_color{0xFF888888};
    };
} // namespace neko::style
