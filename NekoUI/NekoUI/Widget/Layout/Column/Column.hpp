// 2026-08-10

#pragma once
#include "../../Widget.hpp"

#include "ColumnStyle.hpp"

#include "ColumnDraw.hpp"
#include "ColumnHitTest.hpp"
#include "ColumnLayout.hpp"

namespace neko::widget {
    using neko::behavior::ColumnDraw;
    using neko::behavior::ColumnHitTest;
    using neko::behavior::ColumnLayout;

    class Column final : public Widget {
    public:
        explicit Column(engine::Context&) {
            set_geometry(geometry_);
            add_behavior<ColumnLayout>(geometry_, style_);
            add_behavior<ColumnDraw>(geometry_, style_);
            add_behavior<ColumnHitTest>(geometry_);
        }

        auto style() -> style::ColumnStyle& {
            return style_;
        }
    private:
        style::ColumnStyle style_{};
        behavior::GeometryState geometry_{};
    };
} // namespace neko::widget
