// 2026-08-10

#pragma once
#include "../Widget.hpp"

#include "Row/RowStyle.hpp"

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
            add_behavior<RowLayout>(style_);
            add_behavior<RowDraw>(style_);
            add_behavior<RowHitTest>();
        }

        auto style() -> style::RowStyle& {
            return style_;
        }
    private:
        style::RowStyle style_{};
    };
} // namespace neko::widget
