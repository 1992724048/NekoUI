#pragma once
#include <functional>
#include <memory>
#include <shared_mutex>

#include "../Device/Keyboard.hpp"
#include "../Device/Mouse.hpp"
#include "../Platform/Event.hpp"
#include "../Style/ColorScheme.hpp"

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    struct Context {
        std::function<void()> mark_dirty;

        std::function<void(std::weak_ptr<widget::Widget>)> widget_dirty;

        std::function<void()> anim_inc;
        std::function<void()> anim_dec;

        std::function<void()> widget_tree_changed;

        std::weak_ptr<device::Mouse> mouse;
        std::weak_ptr<device::Keyboard> keyboard;

        std::shared_mutex* tree_mutex{nullptr};

        std::weak_ptr<widget::Widget> root;

        style::ColorScheme scheme;
        type::Handle native_handle;
    };
} // namespace neko::engine
