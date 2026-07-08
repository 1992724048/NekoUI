#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "Device/Keyboard.hpp"
#include "Device/Mouse.hpp"

namespace neko::backend {
    class Backend;
}

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    struct Context {
        std::function<void()> present;
        std::function<void()> mark_dirty;

        float dpi_scale = 1.0F;

        std::weak_ptr<mouse::Mouse> mouse;
        std::weak_ptr<keyboard::Keyboard> keyboard;
    };
} // namespace neko::engine
