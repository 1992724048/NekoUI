#include "WidgetTree.hpp"

#include <mutex>
#include <sstream>
#include <utility>

#include "../Widget/Widget.hpp"

namespace neko::engine {
    WidgetTree::WidgetTree() = default;

    auto WidgetTree::set_root(std::shared_ptr<widget::Widget> w) -> void {
        root_ = std::move(w);
        build();
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

    auto WidgetTree::next_focus() -> std::weak_ptr<widget::Widget> {
        if (index_widgets_.empty()) {
            return {};
        }
        if (focus_index > index_count) {
            focus_index = -1;
        }
        return index_widgets_[++focus_index];
    }

    auto WidgetTree::prev_focus() -> std::weak_ptr<widget::Widget> {
        if (index_widgets_.empty()) {
            return {};
        }
        if (focus_index <= 0) {
            focus_index = index_count + 1;
        }
        return index_widgets_[--focus_index];
    }

    auto WidgetTree::clear() -> void {
        root_ = nullptr;
        focused_ = std::weak_ptr<widget::Widget>{};
        std::unique_lock _(mutex_);
        id_widgets_.clear();
        widgets.clear();
    }

    auto WidgetTree::build() -> void {
        std::unique_lock _(mutex_);
        id_widgets_.clear();
        index_widgets_.clear();
        index_count = 0;

        auto traverse = [&](auto& self, MutableWidget& mw, const std::string& parent_path) -> void {
            if (mw.is_widget()) {
                const auto sp = mw.as_widget().lock();
                if (sp) {
                    sp->z_index_ = index_count++;
                    index_widgets_[sp->z_index_] = mw.as_widget();

                    std::ostringstream oss;
                    oss << sp->id_ << "#0x" << std::hex << reinterpret_cast<uintptr_t>(sp.get());
                    sp->path_ = parent_path.empty() ? "/" + oss.str() : parent_path + "/" + oss.str();

                    if (!sp->id_.empty()) {
                        id_widgets_[sp->id_] = mw.as_widget();
                    }
                }
            } else if (mw.is_list()) {
                for (auto& child : mw.as_list()) {
                    self(self, child, parent_path);
                }
            } else if (mw.is_vector()) {
                for (auto& child : mw.as_vector()) {
                    self(self, child, parent_path);
                }
            }
        };

        for (auto& mw : widgets) {
            traverse(traverse, mw, "");
        }
    }
}
