#include "HitTester.hpp"
#include "TreeManager.hpp"
#include "../Device/Mouse.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    HitTester::HitTester(TreeManager& tree) : tree_(tree) {}

    auto HitTester::hit_test(const device::Mouse& mouse) const -> bool {
        std::shared_lock _(tree_.mutex_);

        const auto root = tree_.root_.load();
        if (!root) {
            return false;
        }

        auto test_recursive = [&](auto& self, widget::Widget& w, type::Vec4I parent_rect) -> bool {
            w.set_bounds(parent_rect);

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
                for (auto& mw : children.as_list()) {
                    if (mw.is_widget() && mw.as_widget()) {
                        type::Vec4I child_rect{{{parent_rect.x, offset_y, {parent_rect.z}, {offset_y + 50}}}};
                        if (self(self, *mw.as_widget(), child_rect)) {
                            return true;
                        }
                        offset_y += 50;
                    }
                }
            } else if (children.is_vector()) {
                for (auto& mw : children.as_vector()) {
                    if (mw.is_widget() && mw.as_widget()) {
                        type::Vec4I child_rect{{{parent_rect.x, offset_y, {parent_rect.z}, {offset_y + 50}}}};
                        if (self(self, *mw.as_widget(), child_rect)) {
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
}
