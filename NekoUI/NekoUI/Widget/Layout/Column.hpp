// 2026-08-10

#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"

#include "ColumnDraw.hpp"
#include "ColumnHitTest.hpp"
#include "ColumnLayout.hpp"

namespace neko::widget {
    class Column final : public Widget, public style::BackgroundStyle, public style::SizeStyle {
    public:
        explicit Column(engine::Context&) {
            add_behavior<ColumnLayout>();
            add_behavior<ColumnDraw>();
            add_behavior<ColumnHitTest>();
        }
    };
} // namespace neko::widget
