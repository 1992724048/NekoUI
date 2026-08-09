// 2026-08-10

#pragma once
#include "Behavior.hpp"

namespace neko::device {
    struct Mouse;
}

namespace neko::widget {
    class HitTestBehavior : public Behavior {
    public:
        explicit HitTestBehavior(Widget& owner) : Behavior{owner} {}
        ~HitTestBehavior() override = default;
        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool = 0;
    };
}
