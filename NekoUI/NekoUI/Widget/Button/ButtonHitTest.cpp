// 2026-08-10

#include "ButtonHitTest.hpp"

#include "../../Device/Mouse.hpp"
#include "../Widget.hpp"

namespace neko::widget {
    auto ButtonHitTest::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(owner_.get_bounds());
    }
} // namespace neko::widget
