#include "Checkbox.hpp"

neko::widget::Checkbox::Checkbox(const glm::ivec4 bounds, std::string label) : m_label(std::move(label)) {
    set_bounds(bounds);
    // Set target to match initial checked state
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
}

auto neko::widget::Checkbox::on_update(engine::Context& context) -> void {
    m_bg_anim.set_context(context);
}

auto neko::widget::Checkbox::on_animate(const std::chrono::milliseconds dt) -> void {
    m_bg_anim.update(dt);
}

auto neko::widget::Checkbox::on_draw(engine::Context& context, backend::Backend& backend) -> void {
    const auto& b = bounds();
    constexpr int box = 18;
    const int bx = b.x;
    const int by = b.y + (b.w - box) / 2;

    const glm::vec4 current_f = m_bg_anim;

    // Box background (animated color)
    const glm::ivec4 box_color{static_cast<int>(current_f.r * 255.0F + 0.5F), static_cast<int>(current_f.g * 255.0F + 0.5F), static_cast<int>(current_f.b * 255.0F + 0.5F), 255,};

    if (m_checked || m_bg_anim.animating()) {
        backend.draw_rect_fill({bx, by, box, box}, box_color);
    }

    const auto border = has_focus() ? m_focus_border : m_normal_border;
    backend.draw_rect({bx, by, box, box}, border, 1);

    // Checkmark (white rect approximation)
    if (m_checked || m_bg_anim.animating()) {
        backend.draw_rect_fill({bx + 4, by + 4, box - 8, box - 8}, m_check_color);
    }

    // Label (vertically centered, baseline offset)
    if (!m_label.empty()) {
        constexpr int font_ascent = 13;
        const int label_y = b.y + (b.w - 16) / 2 + font_ascent;
        backend.draw_text(m_label, {bx + box + 8, label_y}, m_text_color);
    }

    // Focus border
    if (has_focus()) {
        backend.draw_rect(b, m_focus_border, 1);
    }
}

auto neko::widget::Checkbox::on_handle(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
    if (msg == WM_LBUTTONDOWN && context.mouse.is_inside(bounds(), context.dpi_scale)) {
        if (context.request_focus) {
            context.request_focus(this);
        }
        toggle(context);
        return true;
    }
    if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
        toggle(context);
        return true;
    }
    return false;
}

auto neko::widget::Checkbox::focusable() const -> bool {
    return true;
}

auto neko::widget::Checkbox::is_checked() const -> bool {
    return m_checked;
}

auto neko::widget::Checkbox::set_checked(const bool checked) -> void {
    m_checked = checked;
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
}

auto neko::widget::Checkbox::toggle() -> void {
    m_checked = !m_checked;
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
    if (on_toggled) {
        on_toggled(m_checked);
    }
}

auto neko::widget::Checkbox::toggle(engine::Context& context) -> void {
    m_checked = !m_checked;
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color; // handles animation_start via ctx_
    context.dirty = true;
    if (on_toggled) {
        on_toggled(m_checked);
    }
}
