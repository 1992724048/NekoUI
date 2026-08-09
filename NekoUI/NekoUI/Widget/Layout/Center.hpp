// 2026-08-10

#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"

#include "CenterDraw.hpp"
#include "CenterHitTest.hpp"
#include "CenterLayout.hpp"

namespace neko::widget {
    class Center final : public Widget, public style::BackgroundStyle {
    public:
        explicit Center(engine::Context&) {
            add_behavior<CenterLayout>();
            add_behavior<CenterDraw>();
            add_behavior<CenterHitTest>();
        }
    };
} // namespace neko::widget
