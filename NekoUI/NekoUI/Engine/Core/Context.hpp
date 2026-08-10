// 2026-08-10 10:18:01

#pragma once
#include <functional>
#include <memory>

#include "../../Device/Keyboard.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Style/ColorScheme.hpp"

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    class TreeManager;

    struct Context : std::enable_shared_from_this<Context> {
        std::function<void()> mark_dirty;

        std::function<void(std::weak_ptr<widget::Widget>)> widget_dirty;

        std::function<void()> anim_inc;
        std::function<void()> anim_dec;

        std::function<void()> widget_tree_changed;

        std::function<void(int pos_x, int pos_y)> set_ime_pos;

        std::weak_ptr<device::Mouse> mouse;
        std::weak_ptr<device::Keyboard> keyboard;

        std::weak_ptr<TreeManager> tree_manager;

        std::weak_ptr<widget::Widget> root;

        style::ColorScheme scheme;
        type::Handle native_handle;
    };
} // namespace neko::engine
