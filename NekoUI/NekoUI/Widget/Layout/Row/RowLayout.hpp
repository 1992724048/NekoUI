// 2026-08-10

#pragma once
#include "../../../Behavior/LayoutBehavior.hpp"

#include "RowStyle.hpp"

namespace neko::behavior {
    class RowLayout final : public LayoutBehavior {
    public:
        RowLayout(neko::widget::Widget& owner, const style::RowStyle& style);
        auto layout(Vec4I rect, engine::Context& context) -> void override;
    private:
        const style::RowStyle& style_;
    };
}
