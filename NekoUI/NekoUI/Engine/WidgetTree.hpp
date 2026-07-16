#pragma once

#include <atomic>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <vector>

#include "MutableWidget.hpp"

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    class WidgetTree {
    public:
        WidgetTree();

        auto set_root(std::shared_ptr<widget::Widget> w) -> void;
        auto get_root() const -> std::shared_ptr<widget::Widget>;

        auto set_focus(std::weak_ptr<widget::Widget> w) -> void;
        auto get_focus() const -> std::weak_ptr<widget::Widget>;

        auto clear() -> void;
        auto build() -> void;
    private:
        std::vector<MutableWidget> widgets;

        std::atomic_int index_count{0};

        std::atomic<std::shared_ptr<widget::Widget>> root_;
        std::atomic<std::weak_ptr<widget::Widget>> focused_;

        std::map<std::string, std::weak_ptr<widget::Widget>> id_widgets_;
        mutable std::shared_mutex mutex_;
    };
}
