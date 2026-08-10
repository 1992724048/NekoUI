// 2026-08-10 06:01:32

#include "WidgetBuilder.hpp"
#include "../Core/Context.hpp"
#include "WidgetVisitor.hpp"

#include <mutex>
#include <utility>

namespace neko::engine {
    WidgetBuilder::WidgetBuilder(std::weak_ptr<TreeManager> tree) :
        tree_(std::move(tree)) {}

    auto WidgetBuilder::build(Context& context) -> void {
        const auto tree = tree_.lock();
        if (!tree) {
            return;
        }
        std::unique_lock _(tree->mutex_);
        tree->id_widgets_.clear();
        tree->index_widgets_.clear();
        tree->index_count = 0;

        const auto root = tree->root_.load();
        if (!root) {
            return;
        }

        tree->register_widget(root, context);

        auto build_recursive = [&](auto& self, widget::Widget& w) -> void {
            visit_children(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               tree->register_widget(child, context);
                               self(self, *child);
                           });
        };
        build_recursive(build_recursive, *root);
    }
}
