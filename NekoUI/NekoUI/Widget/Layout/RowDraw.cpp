// 2026-08-10

#include "RowDraw.hpp"

#include "../../Backend/DirectX11/DirectX11.hpp"
#include "../../Style/CSS.hpp"
#include "../Widget.hpp"

namespace neko::widget {
    RowDraw::RowDraw(Widget& owner, const style::RowStyle& style) :
        DrawBehavior{owner},
        style_{style} {}

    auto RowDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        const auto bounds = owner_.get_bounds();
        const auto bg = style_.background.color.value != 0 ? style_.background : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }
} // namespace neko::widget
