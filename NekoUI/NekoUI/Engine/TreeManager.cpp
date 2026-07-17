#include "TreeManager.hpp"

#include <mutex>
#include <sstream>
#include <utility>

#include "Context.hpp"
#include "WidgetVisitor.hpp"

#include "../Widget/Widget.hpp"

namespace neko::engine {
    TreeManager::TreeManager() = default;

    auto TreeManager::set_root(Context& context, std::shared_ptr<widget::Widget> w) -> void {
        w->context_ = &context;
        root_ = std::move(w);
    }

    auto TreeManager::get_root() const -> std::shared_ptr<widget::Widget> {
        return root_.load();
    }

    auto TreeManager::set_focus(std::weak_ptr<widget::Widget> w) -> void {
        focused_ = std::move(w);
    }

    auto TreeManager::get_focus() const -> std::weak_ptr<widget::Widget> {
        return focused_.load();
    }

    auto TreeManager::next_focus() -> std::weak_ptr<widget::Widget> {
        if (index_widgets_.empty()) {
            return {};
        }
        if (focus_index > index_count) {
            focus_index = -1;
        }
        return index_widgets_[++focus_index];
    }

    auto TreeManager::prev_focus() -> std::weak_ptr<widget::Widget> {
        if (index_widgets_.empty()) {
            return {};
        }
        if (focus_index <= 0) {
            focus_index = index_count + 1;
        }
        return index_widgets_[--focus_index];
    }

    auto TreeManager::clear() -> void {
        root_ = nullptr;
        focused_ = std::weak_ptr<widget::Widget>{};
        std::unique_lock _(mutex_);
        id_widgets_.clear();
        index_widgets_.clear();
    }

    auto TreeManager::event(Context& context) const -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.event(context);
        });
    }

    auto TreeManager::input(Context& context, const platform::Event& event) const -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.input(context, event);
        });
    }

    auto TreeManager::for_each(const std::function<void(widget::Widget&)>& callback) const -> void {
        std::shared_lock _(mutex_);

        const auto root = root_.load();
        if (!root) {
            return;
        }

        callback(*root);

        auto for_each_recursive = [&](auto& self, widget::Widget& w) -> void {
            visit_children(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               callback(*child);
                               self(self, *child);
                           });
        };
        for_each_recursive(for_each_recursive, *root);
    }

    auto TreeManager::register_widget(const std::shared_ptr<widget::Widget>& sp, Context& context) -> void {
        sp->context_ = &context;
        sp->z_index_ = index_count++;
        index_widgets_[sp->z_index_] = sp;

        std::ostringstream oss;
        oss << "/" << sp->id_ << "#0x" << std::hex << reinterpret_cast<uintptr_t>(sp.get());
        sp->path_ = oss.str();

        if (!sp->id_.empty()) {
            id_widgets_[sp->id_] = sp;
        }

        sp->build(context);
    }
}
