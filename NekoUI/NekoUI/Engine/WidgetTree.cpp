#include "WidgetTree.hpp"

#include <mutex>
#include <utility>

#include "../Widget/Widget.hpp"

namespace neko::engine {
    WidgetTree::WidgetTree() = default;

    auto WidgetTree::set_root(std::shared_ptr<widget::Widget> w) -> void {
        root_ = std::move(w);
    }

    auto WidgetTree::get_root() const -> std::shared_ptr<widget::Widget> {
        return root_.load();
    }

    auto WidgetTree::set_focus(std::weak_ptr<widget::Widget> w) -> void {
        focused_ = std::move(w);
    }

    auto WidgetTree::get_focus() const -> std::weak_ptr<widget::Widget> {
        return focused_.load();
    }

    auto WidgetTree::register_widget(const std::weak_ptr<widget::Widget>& w) -> void {
        if (w.expired()) {
            return;
        }

        const auto locked = w.lock();

        std::unique_lock _(id_map_mutex_);
        id_widgets_[locked->id()] = locked;
    }

    auto WidgetTree::unregister_widget(const std::weak_ptr<widget::Widget>& w) -> void {
        if (w.expired()) {
            return;
        }

        const auto locked = w.lock();

        std::unique_lock _(id_map_mutex_);
        if (const auto it = id_widgets_.find(locked->id()); it != id_widgets_.end()) {
            id_widgets_.erase(it);
        }
    }

    auto WidgetTree::clear() -> void {
        root_ = nullptr;
        focused_ = std::weak_ptr<widget::Widget>{};
        std::unique_lock _(id_map_mutex_);
        id_widgets_.clear();
    }
} // namespace neko::engine
