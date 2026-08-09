// 2026-08-10

#pragma once
#include "../Widget.hpp"

#include "../../Style/CSS.hpp"

#include "ColumnDraw.hpp"
#include "ColumnHitTest.hpp"
#include "ColumnLayout.hpp"

namespace neko::widget {
    class Column final : public Widget {
    public:
        explicit Column(engine::Context&) {
            add_behavior<ColumnLayout>(style_);
            add_behavior<ColumnDraw>(style_);
            add_behavior<ColumnHitTest>();
        }

        auto style() -> style::ColumnStyle& {
            return style_;
        }
    private:
        style::ColumnStyle style_{};
    };
} // namespace neko::widget
