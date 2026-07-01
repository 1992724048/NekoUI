#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "Device/Keyboard.hpp"
#include "Device/Mouse.hpp"

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    struct Context {
        std::function<void()> rebuild;
        std::function<void()> rerender;
        std::function<void()> animation_start;
        std::function<void()> animation_end;
        std::function<void(widget::Widget*)> request_focus;

        std::atomic_bool dirty;
        float dpi_scale = 1.0F;

        mouse::Mouse mouse;
        keyboard::Keyboard keyboard;
    };
} // namespace neko::engine
