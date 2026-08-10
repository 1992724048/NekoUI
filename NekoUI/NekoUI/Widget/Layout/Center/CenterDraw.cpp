// 2026-08-10 10:20:09

#include "CenterDraw.hpp"

#include "../../../Backend/DirectX11.hpp"
#include "CenterStyle.hpp"
#include "../../Widget.hpp"

namespace neko::behavior {
    CenterDraw::CenterDraw(widget::Widget& owner, const GeometryState& geometry, const style::CenterStyle& style) :
        DrawBehavior{owner},
        geometry_{geometry},
        style_{style} {}

    auto CenterDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        const auto bounds = geometry_.bounds;
        const auto bg = style_.background.color.value != 0 ? style_.background : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }
} // namespace neko::behavior
