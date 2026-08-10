// 2026-08-10

#pragma once
#include "../../../Behavior/HitTestBehavior.hpp"
#include "../../../Behavior/GeometryState.hpp"

namespace neko::behavior {
    class ColumnHitTest final : public HitTestBehavior {
    public:
        explicit ColumnHitTest(neko::widget::Widget& owner, const behavior::GeometryState& geometry) : HitTestBehavior{owner}, geometry_{geometry} {}
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    private:
        const behavior::GeometryState& geometry_;
    };
}
