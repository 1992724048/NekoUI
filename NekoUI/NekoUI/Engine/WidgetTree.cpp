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

    auto WidgetTree::clear() -> void {
        root_ = nullptr;
        focused_ = std::weak_ptr<widget::Widget>{};
        std::unique_lock _(mutex_);
        id_widgets_.clear();
        widgets.clear();
    }

    auto WidgetTree::build() -> void {
        index_count = 0;

        auto traverse = [&](auto& self, MutableWidget& mw) -> void {
            if (mw.is_widget()) {
                const auto sp = mw.as_widget().lock();
                if (sp) {
                    sp->z_index_ = index_count++;
                }
            } else if (mw.is_list()) {
                for (auto& child : mw.as_list()) {
                    self(self, child);
                }
            } else if (mw.is_vector()) {
                for (auto& child : mw.as_vector()) {
                    self(self, child);
                }
            }
        };

        for (auto& mw : widgets) {
            traverse(traverse, mw);
        }
    }
}
