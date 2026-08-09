// 2026-08-10

#include "ColumnDraw.hpp"

#include "../../Backend/DirectX11/DirectX11.hpp"
#include "../../Style/CSS.hpp"
#include "Column.hpp"

namespace neko::widget {
    auto ColumnDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        auto& column = static_cast<Column&>(owner_);
        const auto bounds = column.get_bounds();
        auto bg = column.background_.color.value != 0 ? column.background_ : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }
} // namespace neko::widget
