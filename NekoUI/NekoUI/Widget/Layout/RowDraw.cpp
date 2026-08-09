// 2026-08-10

#include "RowDraw.hpp"

#include "../../Backend/DirectX11/DirectX11.hpp"
#include "../../Style/CSS.hpp"
#include "Row.hpp"

namespace neko::widget {
    auto RowDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        auto& row = static_cast<Row&>(owner_);
        const auto bounds = row.get_bounds();
        const auto bg = row.background_.color.value != 0 ? row.background_ : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }
} // namespace neko::widget
