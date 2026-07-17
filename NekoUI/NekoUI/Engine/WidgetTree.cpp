#include "WidgetTree.hpp"

#include <mutex>
#include <sstream>
#include <utility>

#include "../Widget/Widget.hpp"

namespace neko::engine {

    template<typename Visitor>
    auto WidgetTree::for_each_child(widget::Widget& w, Visitor&& visit) -> void {
        auto& children = w.get_children();
        if (children.is_null()) return;

        if (children.is_widget()) {
            visit(children.as_widget());
        } else if (children.is_list()) {
            for (auto& mw : children.as_list()) {
                if (mw.is_widget()) visit(mw.as_widget());
            }
        } else if (children.is_vector()) {
            for (auto& mw : children.as_vector()) {
                if (mw.is_widget()) visit(mw.as_widget());
            }
        }
    }

    WidgetTree::WidgetTree() = default;

    auto WidgetTree::set_root(Context& context, std::shared_ptr<widget::Widget> w) -> void {
        w->context_ = &context;
        root_ = std::move(w);
        build(context);
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
        index_widgets_.clear();
    }

    auto WidgetTree::build(Context& context) -> void {
        std::unique_lock _(mutex_);
        id_widgets_.clear();
        index_widgets_.clear();
        index_count = 0;

        const auto root = root_.load();
        if (!root) {
            return;
        }

        register_widget(root, context);

        auto build_recursive = [&](auto& self, widget::Widget& w) -> void {
            for_each_child(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               register_widget(child, context);
                               self(self, *child);
                           });
        };
        build_recursive(build_recursive, *root);
    }

    auto WidgetTree::event(Context& context) const -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.event(context);
        });
    }

    auto WidgetTree::input(Context& context, const platform::Event& event) const -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.input(context, event);
        });
    }

    auto WidgetTree::render(const type::Vec4I rect, Context& context, backend::Backend& backend) const -> void {
        std::shared_lock _(mutex_);

        const auto root = root_.load();
        if (!root) {
            return;
        }

        auto render_recursive = [&](auto& self, widget::Widget& w, const type::Vec4I parent_rect) -> type::Rect {
            w.bounds = parent_rect;
            const auto child_rect = w.draw(parent_rect, context, backend);

            const auto is_horizontal = w.horizontal_;
            auto offset = is_horizontal ? child_rect.x : child_rect.y;

            auto next_rect_for = [&](const int off) -> type::Vec4I {
                if (is_horizontal) {
                    return {{{off, child_rect.y, {off + (child_rect.z - child_rect.x)}, {child_rect.w}}}};
                }
                return {{{child_rect.x, off, {child_rect.z}, {off + (child_rect.w - child_rect.y)}}}};
            };

            for_each_child(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               auto used = self(self, *child, next_rect_for(offset));
                               offset += is_horizontal ? used.width : used.height;
                           });

            return child_rect;
        };

        render_recursive(render_recursive, *root, rect);
    }

    auto WidgetTree::hit_test(const device::Mouse& mouse) const -> bool {
        std::shared_lock _(mutex_);

        const auto root = root_.load();
        if (!root) {
            return false;
        }

        auto test_recursive = [&](auto& self, widget::Widget& w, type::Vec4I parent_rect) -> bool {
            w.bounds = parent_rect;

            auto& children = w.get_children();
            auto offset_y = parent_rect.y;

            if (children.is_widget()) {
                if (const auto& child = children.as_widget()) {
                    type::Vec4I child_rect{{{parent_rect.x, offset_y, {parent_rect.z}, {offset_y + (parent_rect.w - parent_rect.y)}}}};
                    if (self(self, *child, child_rect)) {
                        return true;
                    }
                }
            } else if (children.is_list()) {
                for (auto it = children.as_list().rbegin(); it != children.as_list().rend(); ++it) {
                    if (it->is_widget() && it->as_widget()) {
                        type::Vec4I child_rect{{{parent_rect.x, offset_y, {parent_rect.z}, {offset_y + 50}}}};
                        if (self(self, *it->as_widget(), child_rect)) {
                            return true;
                        }
                        offset_y += 50;
                    }
                }
            } else if (children.is_vector()) {
                for (auto it = children.as_vector().rbegin(); it != children.as_vector().rend(); ++it) {
                    if (it->is_widget() && it->as_widget()) {
                        type::Vec4I child_rect{{{parent_rect.x, offset_y, {parent_rect.z}, {offset_y + 50}}}};
                        if (self(self, *it->as_widget(), child_rect)) {
                            return true;
                        }
                        offset_y += 50;
                    }
                }
            }

            return w.hit_test(mouse);
        };

        return test_recursive(test_recursive, *root, {});
    }

    auto WidgetTree::for_each(const std::function<void(widget::Widget&)>& callback) const -> void {
        std::shared_lock _(mutex_);

        const auto root = root_.load();
        if (!root) {
            return;
        }

        callback(*root);

        auto for_each_recursive = [&](auto& self, widget::Widget& w) -> void {
            for_each_child(w,
                           [&](const std::shared_ptr<widget::Widget>& child) -> void {
                               callback(*child);
                               self(self, *child);
                           });
        };
        for_each_recursive(for_each_recursive, *root);
    }

    auto WidgetTree::register_widget(const std::shared_ptr<widget::Widget>& sp, Context& context) -> void {
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
