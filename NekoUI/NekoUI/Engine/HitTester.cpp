#include "HitTester.hpp"
#include "TreeManager.hpp"
#include "../Device/Mouse.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    HitTester::HitTester(TreeManager& tree) :
        tree_(tree) {}

    auto HitTester::hit_test(const device::Mouse& mouse) const -> widget::Widget* {
        std::shared_lock _(tree_.mutex_);

        const auto root = tree_.root_.load();
        if (!root) {
            return nullptr;
        }

        auto test_recursive = [&](auto& self, widget::Widget& w) -> widget::Widget* {
            auto& children = w.get_children();

            if (children.is_widget()) {
                if (children.as_widget()) {
                    if (auto* hit = self(self, *children.as_widget())) {
                        return hit;
                    }
                }
            } else if (children.is_list()) {
                for (auto it = children.as_list().rbegin(); it != children.as_list().rend(); ++it) {
                    if (it->is_widget() && it->as_widget()) {
                        if (auto* hit = self(self, *it->as_widget())) {
                            return hit;
                        }
                    }
                }
            } else if (children.is_vector()) {
                for (auto it = children.as_vector().rbegin(); it != children.as_vector().rend(); ++it) {
                    if (it->is_widget() && it->as_widget()) {
                        if (auto* hit = self(self, *it->as_widget())) {
                            return hit;
                        }
                    }
                }
            }

            return w.hit_test(mouse) ? &w : nullptr;
        };

        return test_recursive(test_recursive, *root);
    }
}
