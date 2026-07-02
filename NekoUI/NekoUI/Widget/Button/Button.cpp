#include "Button.hpp"

neko::widget::Button::Button(const glm::ivec4 rect, std::string label) : label_(std::move(label)) {
    set_bounds(rect);
    fill_color_.set_observer([this](const glm::vec4&) { mark_dirty(); });
}

auto neko::widget::Button::update(engine::Context& context) -> void {
    fill_color_.set_context(context);
    const float s = context.dpi_scale;
    const bool hover = context.mouse.is_inside(bounds(), s);
    const bool down = hover && context.mouse.left_down;

    if (context.mouse.left_released() && hover && on_click) {
        on_click();
    }

    glm::vec4 new_target;
    if (down) {
        new_target = press_f;
    } else if (hover) {
        new_target = hover_f;
    } else {
        new_target = idle_f;
    }

    if (target_ != new_target) {
        target_ = new_target;
        fill_color_ = new_target;  // triggers animation (handles animation_start via ctx_)
    }

    Widget::update(context);
}

auto neko::widget::Button::animate(const std::chrono::milliseconds dt) -> void {
    fill_color_.update(dt);
    Widget::animate(dt);
}

auto neko::widget::Button::draw(engine::Context& context, backend::Backend& backend) -> void {
    const glm::vec4 current_f = fill_color_;

    if (fill_color_.animating()) {
        context.dirty = true;
    }

    const type::Color current{
        static_cast<int>(current_f.r * 255.0F + 0.5F),
        static_cast<int>(current_f.g * 255.0F + 0.5F),
        static_cast<int>(current_f.b * 255.0F + 0.5F),
        static_cast<int>(current_f.a * 255.0F + 0.5F),
    };
    backend.draw_rect_fill(bounds(), current);
    backend.draw_rect(bounds(), border_color_, 2);

    if (!label_.empty()) {
        const auto& b = bounds();
        const int y_center = b.y + (b.w - 16) / 2;
        backend.draw_text(label_, {b.x + 8, y_center}, text_color_);
    }

    Widget::draw(context, backend);
}
