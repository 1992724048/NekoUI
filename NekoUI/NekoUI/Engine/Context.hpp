#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "Device/Keyboard.hpp"
#include "Device/Mouse.hpp"

namespace neko::engine {
    struct Context {
        std::function<void()> rebuild;
        std::function<void()> rerender;
        std::function<void()> animation_start;
        std::function<void()> animation_end;

        mouse::Mouse mouse;
        keyboard::Keyboard keyboard;
    };
} // namespace neko::engine
