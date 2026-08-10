// 2026-08-10 11:03:52

#include "HitTester.hpp"
#include "TreeManager.hpp"
#include "../../Widget/Widget.hpp"

#include <ranges>
#include <utility>

namespace neko::engine {
    HitTester::HitTester(std::weak_ptr<TreeManager> tree) :
        tree_(std::move(tree)) {}

    auto HitTester::hit_test(const device::Mouse& mouse) const -> std::optional<std::shared_ptr<widget::Widget>> {
        const auto tree = tree_.lock();
        if (!tree) {
            return std::nullopt;
        }
        std::shared_lock _(tree->mutex_);

        const auto root = tree->root_.load();
        if (!root) {
            return std::nullopt;
        }

        auto test_recursive = [&](auto& self, const std::shared_ptr<widget::Widget>& w_ptr) -> std::optional<std::shared_ptr<widget::Widget>> {
            auto& w = *w_ptr;

            if (auto& children = w.get_children(); children.is_widget()) {
                if (auto hit = self(self, children.as_widget())) {
                    return hit;
                }
            } else if (children.is_list()) {
                for (auto& it : std::views::reverse(children.as_list())) {
                    if (it.is_widget()) {
                        if (auto hit = self(self, it.as_widget())) {
                            return hit;
                        }
                    }
                }
            } else if (children.is_vector()) {
                for (auto& it : std::views::reverse(children.as_vector())) {
                    if (it.is_widget()) {
                        if (auto hit = self(self, it.as_widget())) {
                            return hit;
                        }
                    }
                }
            }

            if (w.hit_test(mouse)) {
                return std::optional{std::shared_ptr{w_ptr}};
            }
            return std::nullopt;
        };

        return test_recursive(test_recursive, root);
    }
} // namespace neko::engine
