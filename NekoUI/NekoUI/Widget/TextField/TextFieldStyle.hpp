// 2026-08-10

#pragma once
#include "../../Style/CSS.hpp"

namespace neko::style {
    struct TextFieldStyle {
        Background background;              // 0 = 回退 scheme.surface
        Size size;                          // 哨兵 = 父尺寸
        Border border;                      // width/color
        Text text{.color = type::Color{0}}; // 0 = 回退 scheme.on_surface（CSS Text 默认不透明白）
        type::Color caret_color{};          // 0 = 回退 scheme.on_surface
        type::Color caret_color_focus{0xFF0000FF};  // 聚焦光标保持蓝色
        type::Color comp_color{0xFF888888};         // IME 合成预览色
    };
} // namespace neko::style
