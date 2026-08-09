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
    class TreeManager;

    // enable_shared_from_this：TreeManager 需要从 Context& 取得 shared_ptr 以填充 Widget::context_；
    // 前提是 Context 由 Engine 以 shared_ptr 持有（否则 shared_from_this 抛 bad_weak_ptr）
    struct Context : public std::enable_shared_from_this<Context> {
        std::function<void()> mark_dirty;

        std::function<void(std::weak_ptr<widget::Widget>)> widget_dirty;

        std::function<void()> anim_inc;
        std::function<void()> anim_dec;

        std::function<void()> widget_tree_changed;

        std::weak_ptr<device::Mouse> mouse;
        std::weak_ptr<device::Keyboard> keyboard;

        std::weak_ptr<TreeManager> tree_manager;

        std::weak_ptr<widget::Widget> root;

        style::ColorScheme scheme;
        type::Handle native_handle;
    };
} // namespace neko::engine
