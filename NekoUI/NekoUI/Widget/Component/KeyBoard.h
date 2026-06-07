#pragma once
#include <bitset>
#include "Event.hpp"

namespace neko::event {
    using namespace type;

    class KeyBoard : Event {
    public:
        ~KeyBoard() override = default;
        virtual auto keyboard_button(std::bitset<108> state) -> void {}
    };
} // namespace neko::event
