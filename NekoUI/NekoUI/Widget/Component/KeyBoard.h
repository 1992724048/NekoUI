#pragma once
#include <bitset>

namespace neko::event {
    class KeyBoard {
    public:
        virtual ~KeyBoard() = default;
        virtual auto keyboard_button(std::bitset<108> state) -> void {}
    };
} // namespace neko::event
