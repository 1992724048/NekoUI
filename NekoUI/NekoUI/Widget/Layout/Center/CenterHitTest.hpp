// 2026-08-10

#pragma once
#include "../../../Behavior/HitTestBehavior.hpp"

namespace neko::behavior {
    class CenterHitTest final : public HitTestBehavior {
    public:
        explicit CenterHitTest(neko::widget::Widget& owner) : HitTestBehavior{owner} {}
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
}
