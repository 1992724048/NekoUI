// 2026-08-10

#include "CenterDraw.hpp"

#include "../../Backend/DirectX11/DirectX11.hpp"
#include "../../Style/CSS.hpp"
#include "Center.hpp"

namespace neko::widget {
    auto CenterDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        auto& center = static_cast<Center&>(owner_);
        const auto bounds = center.get_bounds();
        auto bg = center.background_.color.value != 0 ? center.background_ : style::Background{context.scheme.surface};
        if (bg.color.value != 0) {
            backend.draw_rect_fill(bounds, bg.color);
        }
        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }
} // namespace neko::widget
