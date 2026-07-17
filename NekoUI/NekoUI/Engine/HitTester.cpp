#include "HitTester.hpp"
#include "TreeManager.hpp"
#include "../Device/Mouse.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    HitTester::HitTester(TreeManager& tree) :
        tree_(tree) {}

    auto HitTester::hit_test(const device::Mouse& mouse) const -> bool {
        std::shared_lock _(tree_.mutex_);

        const auto root = tree_.root_.load();
        if (!root) return false;

        // 直接使用 render 阶段写好的 bounds，不做重复布局计算
        auto test_recursive = [&](auto& self, widget::Widget& w) -> bool {
            auto& children = w.get_children();

            if (children.is_widget()) {
                if (children.as_widget() && self(self, *children.as_widget())) return true;
            } else if (children.is_list()) {
                for (auto& mw : children.as_list()) {
                    if (mw.is_widget() && mw.as_widget() && self(self, *mw.as_widget())) return true;
                }
            } else if (children.is_vector()) {
                for (auto& mw : children.as_vector()) {
                    if (mw.is_widget() && mw.as_widget() && self(self, *mw.as_widget())) return true;
                }
            }

            return w.hit_test(mouse);
        };

        return test_recursive(test_recursive, *root);
    }
}
