#include "WidgetBuilder.hpp"
#include "Context.hpp"
#include "WidgetVisitor.hpp"

namespace neko::engine {
    WidgetBuilder::WidgetBuilder(TreeManager& tree) : tree_(tree) {}

    auto WidgetBuilder::build(Context& context) -> void {
        std::unique_lock _(tree_.mutex_);
        tree_.id_widgets_.clear();
        tree_.index_widgets_.clear();
        tree_.index_count = 0;

        const auto root = tree_.root_.load();
        if (!root) return;

        tree_.register_widget(root, context);

        auto build_recursive = [&](auto& self, widget::Widget& w) -> void {
            visit_children(w, [&](const std::shared_ptr<widget::Widget>& child) {
                tree_.register_widget(child, context);
                self(self, *child);
            });
        };
        build_recursive(build_recursive, *root);
    }
}
