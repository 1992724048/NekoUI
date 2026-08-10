// 2026-08-10

#include "ButtonDraw.hpp"

#include <utility>

#include "../../Backend/DirectX11/DirectX11.hpp"
#include "../../Style/CSS.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    ButtonDraw::ButtonDraw(neko::widget::Widget& owner, const style::ButtonStyle& style, const engine::Context& context, std::string text) :
        DrawBehavior{owner},
        style_{style},
        text_(std::move(text)) {
        scale_.bind(context.anim_inc, context.anim_dec);
    }

    auto ButtonDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        const bool hovered = owner_.get_hovered();
        if (hovered != prev_hovered_) {
            prev_hovered_ = hovered;
            scale_.to_value(hovered ? 1.06F : 1.0F);
        }

        const auto bg = style_.background.color.value != 0 ? style_.background : style::Background{hovered ? context.scheme.secondary_container : context.scheme.primary};
        const auto tc = style_.text.color.value != 0 ? style_.text.color : Color{0xFFFFFFFF};

        const auto s = scale_.tick();
        const auto bounds = owner_.get_bounds();
        const auto center_x = (bounds.x + bounds.z) / 2;
        const auto center_y = (bounds.y + bounds.w) / 2;
        const auto half_w = static_cast<int>(static_cast<float>(bounds.z - bounds.x) * s / 2.0F);
        const auto half_h = static_cast<int>(static_cast<float>(bounds.w - bounds.y) * s / 2.0F);
        const Vec4I visual{.x = center_x - half_w, .y = center_y - half_h, .z = center_x + half_w, .w = center_y + half_h};

        backend.draw_rect_fill(visual, bg.color);
        if (style_.border.width > 0.0F) {
            backend.draw_rect(visual, style_.border.color, static_cast<int>(style_.border.width));
        }
        if (!text_.empty()) {
            const auto text_pos = Vec2I{.x = visual.x + (visual.z - visual.x) / 10, .y = visual.y + (visual.w - visual.y) / 2};
            backend.draw_text(text_, text_pos, tc, style_.text.font_size);
        }
        return {.x = visual.x, .y = visual.y, .width = visual.z - visual.x, .height = visual.w - visual.y};
    }
} // namespace neko::behavior
