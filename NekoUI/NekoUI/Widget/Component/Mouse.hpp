#pragma once
#include <bitset>

#include "../../Type.hpp"

namespace neko::event {
    using namespace type;

    class Mouse {
    public:
        virtual ~Mouse() = default;
        virtual auto mouse_move(Vec2<int> pos) -> void {}
        virtual auto mouse_button(Vec2<int> pos, std::bitset<7> state) -> void {}
        virtual auto mouse_wheel(Vec2<int> pos, int wheel) -> void {}
    };
} // namespace neko::event
