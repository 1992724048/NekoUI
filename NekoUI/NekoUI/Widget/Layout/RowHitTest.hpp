// 2026-08-10

#pragma once
#include "../Behavior/HitTestBehavior.hpp"

namespace neko::widget {
    class RowHitTest final : public HitTestBehavior {
    public:
        explicit RowHitTest(Widget& owner) : HitTestBehavior{owner} {}
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
}
