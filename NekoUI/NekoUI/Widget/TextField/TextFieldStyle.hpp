// 2026-08-10

#pragma once
#include "../../Style/CSS.hpp"

namespace neko::style {
    struct TextFieldStyle {
        Background background;              // 0 = 回退 scheme.surface
        Size size;                          // 哨兵 = 父尺寸
        Border border;                      // width/color
        Text text;                          // 文本色/字号
        type::Color caret_color{0xFF000000};        // 光标色
        type::Color caret_color_focus{0xFF0000FF};  // 聚焦光标色
        type::Color comp_color{0xFF888888};         // IME 合成预览色
    };
} // namespace neko::style
