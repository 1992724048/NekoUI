// 2026-08-10

#pragma once

#include <string>

namespace neko::behavior {
    // 文本输入状态（Input 写、Draw 读；生命周期 = 持有控件）
    struct TextFieldState {
        std::string text;       // UTF-8 已确认文本
        size_t caret_pos{0};    // 光标位置（码点计数）
        std::string ime_comp;   // IME 合成中文本（UTF-8）
        bool ime_active{false}; // 合成中
    };
} // namespace neko::behavior
