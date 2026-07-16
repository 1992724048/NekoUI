#pragma once
#include <functional>
#include <memory>

#include "../Device/Keyboard.hpp"
#include "../Device/mouse.hpp"
#include "../Platform/Event.hpp"
#include "../Style/ColorScheme.hpp"

namespace neko::backend {
    class Backend;
}

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    struct Context {
        std::function<void()> mark_dirty;

        std::function<void(std::weak_ptr<widget::Widget>)> widget_dirty;

        std::function<void()> anim_inc;
        std::function<void()> anim_dec;

        std::weak_ptr<device::Mouse> mouse;
        std::weak_ptr<device::Keyboard> keyboard;

        std::weak_ptr<widget::Widget> root;

        style::ColorScheme scheme;
        type::Handle native_handle;
    };
} // namespace neko::engine
