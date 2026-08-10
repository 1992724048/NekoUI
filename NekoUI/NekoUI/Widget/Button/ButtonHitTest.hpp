// 2026-08-10

#pragma once
#include "../../Behavior/HitTestBehavior.hpp"

namespace neko::behavior {
    class ButtonHitTest final : public HitTestBehavior {
    public:
        explicit ButtonHitTest(neko::widget::Widget& owner) : HitTestBehavior{owner} {}
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
}
