// 2026-08-10

#pragma once
#include "../../Widget.hpp"

#include "RowStyle.hpp"

#include "RowDraw.hpp"
#include "RowHitTest.hpp"
#include "RowLayout.hpp"

namespace neko::widget {
    using neko::behavior::RowDraw;
    using neko::behavior::RowHitTest;
    using neko::behavior::RowLayout;

    class Row final : public Widget {
    public:
        explicit Row(engine::Context&) {
            set_geometry(geometry_);
            add_behavior<RowLayout>(geometry_, style_);
            add_behavior<RowDraw>(geometry_, style_);
            add_behavior<RowHitTest>(geometry_);
        }

        auto style() -> style::RowStyle& {
            return style_;
        }
    private:
        style::RowStyle style_{};
        behavior::GeometryState geometry_{};
    };
} // namespace neko::widget
