// 2026-08-10

#pragma once
#include "../../Widget.hpp"

#include "CenterStyle.hpp"

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
            set_geometry(geometry_);
            add_behavior<CenterLayout>(geometry_);
            add_behavior<CenterDraw>(geometry_, style_);
            add_behavior<CenterHitTest>(geometry_);
        }

        auto style() -> style::CenterStyle& {
            return style_;
        }
    private:
        style::CenterStyle style_{};
        behavior::GeometryState geometry_{};
    };
} // namespace neko::widget
