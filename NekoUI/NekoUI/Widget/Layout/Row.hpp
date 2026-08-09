// 2026-08-10

#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"

#include "RowDraw.hpp"
#include "RowHitTest.hpp"
#include "RowLayout.hpp"

namespace neko::widget {
    class Row final : public Widget, public style::BackgroundStyle, public style::SizeStyle {
    public:
        explicit Row(engine::Context&) {
            add_behavior<RowLayout>();
            add_behavior<RowDraw>();
            add_behavior<RowHitTest>();
        }
    };
} // namespace neko::widget
