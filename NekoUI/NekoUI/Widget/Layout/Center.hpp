// 2026-08-10

#pragma once
#include "../Widget.hpp"

#include "../../Style/CSS.hpp"

#include "CenterDraw.hpp"
#include "CenterHitTest.hpp"
#include "CenterLayout.hpp"

namespace neko::widget {
    using neko::behavior::CenterDraw;
    using neko::behavior::CenterHitTest;
    using neko::behavior::CenterLayout;

    class Center final : public Widget {
    public:
        explicit Center(engine::Context&) {
            add_behavior<CenterLayout>();
            add_behavior<CenterDraw>(style_);
            add_behavior<CenterHitTest>();
        }

        auto style() -> style::CenterStyle& {
            return style_;
        }
    private:
        style::CenterStyle style_{};
    };
} // namespace neko::widget
