#pragma once
#include <functional>
#include <memory>
#include <vector>

#include "Component/ColorSeed.hpp"
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

        std::function<void()> anim_inc;
        std::function<void()> anim_dec;

        float dpi_scale = 1.0F;

        std::weak_ptr<mouse::Mouse> mouse;
        std::weak_ptr<keyboard::Keyboard> keyboard;
        std::weak_ptr<color::ColorScheme> color_scheme;
    };
} // namespace neko::engine
