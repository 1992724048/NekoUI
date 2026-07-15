#pragma once

#include <atomic>
#include <list>
#include <map>
#include <memory>
#include <shared_mutex>
#include <string>
#include <variant>
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

        auto register_widget(const std::weak_ptr<widget::Widget>& w) -> void;
        auto unregister_widget(const std::weak_ptr<widget::Widget>& w) -> void;

        auto clear() -> void;
    private:
        std::list<MutableWidget> widgets;

        std::atomic<std::shared_ptr<widget::Widget>> root_;
        std::atomic<std::weak_ptr<widget::Widget>> focused_;

        std::map<std::string, std::weak_ptr<widget::Widget>> id_widgets_;
        mutable std::shared_mutex id_map_mutex_;
    };
} // namespace neko::engine
