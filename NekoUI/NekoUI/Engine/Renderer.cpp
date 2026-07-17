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

        auto render_recursive = [&](auto& self, widget::Widget& w, const type::Vec4I parent_rect) -> type::Rect {
            w.set_bounds(parent_rect);
            const auto child_rect = w.draw(parent_rect, context, backend);

            const auto is_horizontal = w.horizontal_;
            auto offset = is_horizontal ? child_rect.x : child_rect.y;

            auto next_rect_for = [&](const int off) -> type::Vec4I {
                if (is_horizontal) {
                    return {{{off, child_rect.y, {off + (child_rect.z - child_rect.x)}, {child_rect.w}}}};
                }
                return {{{child_rect.x, off, {child_rect.z}, {off + (child_rect.w - child_rect.y)}}}};
            };

            visit_children(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               auto used = self(self, *child, next_rect_for(offset));
                               offset += is_horizontal ? used.width : used.height;
                           });

            return child_rect;
        };

        render_recursive(render_recursive, *root, rect);
    }
}
