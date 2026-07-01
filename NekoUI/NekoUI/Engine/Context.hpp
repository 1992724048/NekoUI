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

        std::atomic_bool dirty;
        float dpi_scale = 1.0F;

        mouse::Mouse mouse;
        keyboard::Keyboard keyboard;
    };
} // namespace neko::engine
