#include "WidgetTree.hpp"

#include <mutex>
#include <sstream>
#include <utility>

#include "../Widget/Widget.hpp"

namespace neko::engine {
    WidgetTree::WidgetTree() = default;

    auto WidgetTree::set_root(Context& context, std::shared_ptr<widget::Widget> w) -> void {
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
        widgets.clear();
    }

    auto WidgetTree::build(Context& context) -> void {
        std::unique_lock _(mutex_);
        id_widgets_.clear();
        index_widgets_.clear();
        index_count = 0;

        traverse_impl([&](MutableWidget& mw) -> void {
            const auto& sp = mw.as_widget();
            if (!sp) {
                return;
            }
            register_widget(sp, context);
        });

        const auto root = root_.load();
        if (root) {
            auto build_recursive = [&](auto& self, widget::Widget& w) -> void {
                auto& children = w.get_children();
                if (children.is_null()) {
                    return;
                }

                auto visit_child = [&](const std::shared_ptr<widget::Widget>& child) -> void {
                    if (!child) {
                        return;
                    }
                    register_widget(child, context);
                    self(self, *child);
                };

                if (children.is_widget()) {
                    visit_child(children.as_widget());
                } else if (children.is_list()) {
                    for (auto& mw : children.as_list()) {
                        if (mw.is_widget()) {
                            visit_child(mw.as_widget());
                        }
                    }
                } else if (children.is_vector()) {
                    for (auto& mw : children.as_vector()) {
                        if (mw.is_widget()) {
                            visit_child(mw.as_widget());
                        }
                    }
                }
            };
            build_recursive(build_recursive, *root);
        }
    }

    auto WidgetTree::event(Context& context) -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.event(context);
        });
    }

    auto WidgetTree::input(Context& context, const platform::Event& event) -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.input(context, event);
        });
    }

    auto WidgetTree::render(const type::Vec4I rect, Context& context, backend::Backend& backend) -> void {
        for_each([&](widget::Widget& widget) -> void {
            widget.draw(rect, context, backend);
        });
    }

    auto WidgetTree::hit_test(const device::Mouse& mouse) -> bool {
        bool hit{false};
        for_each([&](const widget::Widget& widget) -> void {
            if (widget.hit_test(mouse)) {
                hit = true;
            }
        });
        return hit;
    }

    auto WidgetTree::for_each(const std::function<void(widget::Widget&)>& callback) -> void {
        std::shared_lock _(mutex_);

        traverse_impl([&](MutableWidget& mw) -> void {
            if (const auto& sp = mw.as_widget()) {
                callback(*sp);
            }
        });
    }

    auto WidgetTree::traverse_impl(const std::function<void(MutableWidget&)>& callback) -> void {
        auto traverse = [&](auto& self, MutableWidget& mw) -> void {
            if (mw.is_widget()) {
                callback(mw);
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

    auto WidgetTree::register_widget(const std::shared_ptr<widget::Widget>& sp, Context& context) -> void {
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
