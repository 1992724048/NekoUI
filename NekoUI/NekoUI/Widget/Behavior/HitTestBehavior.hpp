// 2026-08-10

#pragma once
#include "Behavior.hpp"

namespace neko::device {
    class Mouse;
}

namespace neko::widget {
    class HitTestBehavior : public Behavior {
    public:
        explicit HitTestBehavior(Widget& owner) : Behavior{owner} {}
        virtual ~HitTestBehavior() = default;
        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool = 0;
    };
}
