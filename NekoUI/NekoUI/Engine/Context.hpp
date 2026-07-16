#pragma once
#include <cstdint>
#include <functional>
#include <memory>

#include "../Device/Keyboard.hpp"
#include "../Device/mouse.hpp"
#include "../Platform/Event.hpp"

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

        std::function<void(std::weak_ptr<widget::Widget>)> reg_widget;
        std::function<void(std::weak_ptr<widget::Widget>)> del_widget;

        std::weak_ptr<device::Mouse> mouse;
        std::weak_ptr<device::Keyboard> keyboard;

        std::weak_ptr<widget::Widget> root;

        platform::ThemeMode theme_mode = platform::ThemeMode::Light;
        type::Color theme_color = {.value = 0};
    };
} // namespace neko::engine
