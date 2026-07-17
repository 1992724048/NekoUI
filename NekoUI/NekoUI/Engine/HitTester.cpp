#include "HitTester.hpp"
#include "TreeManager.hpp"
#include "../Device/Mouse.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    HitTester::HitTester(TreeManager& tree) :
        tree_(tree) {}

    auto HitTester::hit_test(const device::Mouse& mouse) const -> std::shared_ptr<widget::Widget> {
        std::shared_lock _(tree_.mutex_);

        const auto root = tree_.root_.load();
        if (!root) {
            return nullptr;
        }

        auto test_recursive = [&](auto& self, const std::shared_ptr<widget::Widget>& w_ptr) -> std::shared_ptr<widget::Widget> {
            auto& w = *w_ptr;

            if (auto& children = w.get_children(); children.is_widget()) {
                if (auto hit = self(self, children.as_widget())) {
                    return hit;
                }
            } else if (children.is_list()) {
                for (auto it = children.as_list().rbegin(); it != children.as_list().rend(); ++it) {
                    if (it->is_widget()) {
                        if (auto hit = self(self, it->as_widget())) {
                            return hit;
                        }
                    }
                }
            } else if (children.is_vector()) {
                for (auto it = children.as_vector().rbegin(); it != children.as_vector().rend(); ++it) {
                    if (it->is_widget()) {
                        if (auto hit = self(self, it->as_widget())) {
                            return hit;
                        }
                    }
                }
            }

            return w.hit_test(mouse) ? w_ptr : nullptr;
        };

        return test_recursive(test_recursive, root);
    }
} // namespace neko::engine
