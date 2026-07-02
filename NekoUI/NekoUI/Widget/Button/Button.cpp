#include "Button.hpp"

neko::widget::Button::Button(const glm::ivec4 rect, std::string label) : label_(std::move(label)) {
    set_bounds(rect);
}

auto neko::widget::Button::update(engine::Context& context) -> void {
    const float s = context.dpi_scale;
    const bool hover = context.mouse.is_inside(bounds(), s);
    const bool down = hover && context.mouse.left_down;

    if (context.mouse.left_released() && hover && on_click) {
        on_click();
    }

    glm::vec4 target;
    if (down) {
        target = press_f;
    } else if (hover) {
        target = hover_f;
    } else {
        target = idle_f;
    }

    if (target != current_target_) {
        current_target_ = target;
        if (anim_state == AnimState::Idle) {
            context.animation_start();
            anim_state = AnimState::Animating;
        }
        color_anim = target;
        context.dirty = true;
    }

    Widget::update(context);
}

auto neko::widget::Button::draw(engine::Context& context, backend::Backend& backend) -> void {
    const glm::vec4 current_f = color_anim();

    if (color_anim.is_done() && anim_state == AnimState::Animating) {
        context.animation_end();
        anim_state = AnimState::Idle;
    }
    const type::Color current{
        static_cast<int>(current_f.r * 255.0F + 0.5F),
        static_cast<int>(current_f.g * 255.0F + 0.5F),
        static_cast<int>(current_f.b * 255.0F + 0.5F),
        static_cast<int>(current_f.a * 255.0F + 0.5F),
    };
    backend.draw_rect_fill(bounds(), current);
    backend.draw_rect(bounds(), border_color, 2);

    if (!label_.empty()) {
        const auto& b = bounds();
        const int y_center = b.y + (b.w - 16) / 2;
        backend.draw_text(label_, {b.x + 8, y_center}, text_color);
    }

    Widget::draw(context, backend);
}
