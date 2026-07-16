#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "MutableWidget.hpp"

#include "../Type.hpp"
#include "../Platform/Platform.hpp"

namespace neko::device {
    struct Mouse;
}

namespace neko::backend {
    class Backend;
}

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    struct Context;

    class WidgetTree {
    public:
        WidgetTree();

        auto set_root(Context& context, std::shared_ptr<widget::Widget> w) -> void;
        auto get_root() const -> std::shared_ptr<widget::Widget>;

        auto set_focus(std::weak_ptr<widget::Widget> w) -> void;
        auto get_focus() const -> std::weak_ptr<widget::Widget>;

        auto next_focus() -> std::weak_ptr<widget::Widget>;
        auto prev_focus() -> std::weak_ptr<widget::Widget>;

        auto clear() -> void;
        auto build(Context& context) -> void;
        auto event(Context& context) -> void;
        auto input(Context& context, const platform::Event& event) -> void;
        auto render(type::Vec4I rect, Context& context, backend::Backend& backend) -> void;
        auto hit_test(const device::Mouse& mouse) -> bool;
        auto for_each(const std::function<void(widget::Widget&)>& callback) -> void;
    private:
        std::list<MutableWidget> widgets;

        int focus_index{0};
        std::atomic_int index_count{0};

        std::atomic<std::shared_ptr<widget::Widget>> root_;
        std::atomic<std::weak_ptr<widget::Widget>> focused_;

        std::map<std::string, std::weak_ptr<widget::Widget>> id_widgets_;
        std::map<int, std::weak_ptr<widget::Widget>> index_widgets_;
        auto traverse_impl(const std::function<void(MutableWidget&)>& callback) -> void;

        mutable std::shared_mutex mutex_;
    };
}
