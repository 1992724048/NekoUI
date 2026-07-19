#include "Renderer.hpp"
#include "Context.hpp"
#include "TreeManager.hpp"
#include "WidgetVisitor.hpp"
#include "../Backend/Backend.hpp"

namespace neko::engine {
    Renderer::Renderer(TreeManager& tree) :
        tree_(tree) {}

    auto Renderer::render(const type::Vec4I rect, Context& context, backend::Backend& backend) const -> void {
        std::shared_lock _(tree_.mutex_);

        const auto root = tree_.root_.load();
        if (!root) {
            return;
        }

        root->layout(rect, context);

        auto draw_recursive = [&](auto& self, widget::Widget& w) -> void {
            w.draw(w.get_bounds(), context, backend);
            visit_children(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               self(self, *child);
                           });
        };
        draw_recursive(draw_recursive, *root);
    }
}
