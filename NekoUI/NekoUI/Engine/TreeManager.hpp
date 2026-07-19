#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>

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

    class TreeManager {
    public:
        TreeManager();

        auto set_root(Context& context, std::shared_ptr<widget::Widget> w) -> void;
        auto get_root() const -> std::shared_ptr<widget::Widget>;

        auto set_focus(std::weak_ptr<widget::Widget> w) -> void;
        auto get_focus() const -> std::weak_ptr<widget::Widget>;

        auto next_focus() -> std::weak_ptr<widget::Widget>;
        auto prev_focus() -> std::weak_ptr<widget::Widget>;

        auto clear() -> void;
        auto event(Context& context) const -> void;
        auto input(Context& context, const platform::Event& event) const -> void;
        auto for_each(const std::function<void(widget::Widget&)>& callback) const -> void;

        auto register_widget(const std::shared_ptr<widget::Widget>& sp, Context& context) -> void;

        int focus_index{0};
        std::atomic_int index_count{0};

        MutableWidget children_{};

        std::atomic<std::shared_ptr<widget::Widget>> root_;
        std::atomic<std::weak_ptr<widget::Widget>> focused_;

        std::map<std::string, std::weak_ptr<widget::Widget>> id_widgets_;
        std::map<int, std::weak_ptr<widget::Widget>> index_widgets_;

        mutable std::shared_mutex mutex_;
    };
}
