// 2026-08-10 10:18:46

#pragma once

#include <string>

namespace neko::behavior {
    struct TextFieldState {
        std::string text;
        size_t caret_pos{0};
        std::string ime_comp;
        bool ime_active{false};
    };
} // namespace neko::behavior
