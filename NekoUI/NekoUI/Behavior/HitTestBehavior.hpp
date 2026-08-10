// 2026-08-10

#pragma once
#include "Behavior.hpp"

namespace neko::device {
    struct Mouse;
}

namespace neko::behavior {
    class HitTestBehavior : public Behavior {
    public:
        explicit HitTestBehavior(neko::widget::Widget& owner) : Behavior{owner} {}
        ~HitTestBehavior() override = default;
        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool = 0;
    };
}
