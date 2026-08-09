// 2026-08-10

#pragma once
#include "../Behavior/HitTestBehavior.hpp"

namespace neko::widget {
    class CenterHitTest final : public HitTestBehavior {
    public:
        explicit CenterHitTest(Widget& owner) : HitTestBehavior{owner} {}
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
}
