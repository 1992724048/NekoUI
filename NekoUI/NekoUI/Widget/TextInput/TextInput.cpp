#include "TextInput.hpp"

neko::widget::TextInput::TextInput(const glm::ivec4 bounds, std::string placeholder) : m_placeholder(std::move(placeholder)) {
    set_bounds(bounds);
}

auto neko::widget::TextInput::on_update(engine::Context& context) -> void {
    const auto now = std::chrono::steady_clock::now();
    if (now - m_cursor_tick > std::chrono::milliseconds(500)) {
        m_cursor_visible = !m_cursor_visible;
        m_cursor_tick = now;
        context.dirty = true;
    }
}

auto neko::widget::TextInput::on_draw(engine::Context& context, backend::Backend& backend) -> void {
    const auto& b = bounds();
    const auto& text = m_text.get();

    // Background + border
    backend.draw_rect_fill(b, m_bg_color);
    backend.draw_rect(b, has_focus() ? m_focus_border_color : m_border_color, 1);

    // Selection background
    if (has_focus() && m_sel_start >= 0 && m_sel_start != m_cursor_pos) {
        const int sel_begin = (std::min)(m_sel_start, m_cursor_pos);
        const int sel_end = (std::max)(m_sel_start, m_cursor_pos);
        backend.draw_rect_fill({b.x + 2, b.y + 2, (sel_end - sel_begin) * 8, b.w - 4}, m_sel_color);
    }

    // Text (vertically centered)
    const bool empty = text.empty();
    const auto& display_text = empty ? m_placeholder : text;
    const auto text_color = empty ? m_placeholder_color : m_text_color;
    constexpr int font_size = 16;
    constexpr int font_ascent = 13;
    const int text_y = b.y + (b.w - font_size) / 2 + font_ascent;
    backend.draw_text(display_text, {b.x + 4, text_y}, text_color);

    // Cursor
    if (has_focus() && m_cursor_visible) {
        int cx = b.x + 4 + (m_cursor_pos * 8);
        backend.draw_rect({cx, b.y + 2, 2, b.w - 4}, m_cursor_color, 1);
    }
}

auto neko::widget::TextInput::on_handle(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
    if (msg == WM_CHAR) {
        const auto ch = static_cast<char>(wparam);
        if (ch >= 32 && ch <= 126) {
            auto new_text = m_text.get();
            new_text.insert(m_cursor_pos, 1, ch);
            m_text = std::move(new_text);
            ++m_cursor_pos;
            m_sel_start = -1;
            if (on_text_changed) {
                on_text_changed(m_text.get());
            }
        }
        return true;
    }

    if (msg == WM_KEYDOWN) {
        switch (wparam) {
            case VK_BACK: {
                auto new_text = m_text.get();
                if (has_selection()) {
                    const int start = (std::min)(m_sel_start, m_cursor_pos);
                    const int end = (std::max)(m_sel_start, m_cursor_pos);
                    new_text.erase(start, end - start);
                    m_cursor_pos = start;
                    m_sel_start = -1;
                } else if (m_cursor_pos > 0 && !new_text.empty()) {
                    new_text.erase(m_cursor_pos - 1, 1);
                    --m_cursor_pos;
                    m_sel_start = -1;
                } else {
                    return true;
                }
                m_text = std::move(new_text);
                m_cursor_visible = true;
                m_cursor_tick = std::chrono::steady_clock::now();
                if (on_text_changed) {
                    on_text_changed(m_text.get());
                }
                return true;
            }
            case VK_DELETE: {
                auto new_text = m_text.get();
                if (has_selection()) {
                    const int start = (std::min)(m_sel_start, m_cursor_pos);
                    const int end = (std::max)(m_sel_start, m_cursor_pos);
                    new_text.erase(start, end - start);
                    m_cursor_pos = start;
                    m_sel_start = -1;
                } else if (std::cmp_less(m_cursor_pos, new_text.size())) {
                    new_text.erase(m_cursor_pos, 1);
                    m_sel_start = -1;
                } else {
                    return true;
                }
                m_text = std::move(new_text);
                m_cursor_visible = true;
                m_cursor_tick = std::chrono::steady_clock::now();
                if (on_text_changed) {
                    on_text_changed(m_text.get());
                }
                return true;
            }
            case VK_LEFT:
                if (m_cursor_pos > 0) {
                    --m_cursor_pos;
                    m_sel_start = -1;
                    context.dirty = true;
                }
                return true;
            case VK_RIGHT:
                if (std::cmp_less(m_cursor_pos, m_text.get().size())) {
                    ++m_cursor_pos;
                    m_sel_start = -1;
                    context.dirty = true;
                }
                return true;
            case VK_HOME:
                m_cursor_pos = 0;
                m_sel_start = -1;
                context.dirty = true;
                return true;
            case VK_END:
                m_cursor_pos = static_cast<int>(m_text.get().size());
                m_sel_start = -1;
                context.dirty = true;
                return true;
            case 'A':
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
                    m_sel_start = 0;
                    m_cursor_pos = static_cast<int>(m_text.get().size());
                    context.dirty = true;
                    return true;
                }
                break;
            default: ;
        }
        return true;
    }

    return false;
}

auto neko::widget::TextInput::focusable() const -> bool {
    return true;
}

auto neko::widget::TextInput::on_focus_gained() -> void {
    m_cursor_visible = true;
    m_cursor_tick = std::chrono::steady_clock::now();
}

auto neko::widget::TextInput::on_focus_lost() -> void {
    m_cursor_visible = false;
    m_sel_start = -1;
}

auto neko::widget::TextInput::text() const -> const std::string& {
    return m_text.get();
}

auto neko::widget::TextInput::set_text(const std::string_view t) -> void {
    m_text = std::string(t);
    m_cursor_pos = static_cast<int>(m_text.get().size());
    m_sel_start = -1;
}

auto neko::widget::TextInput::set_placeholder(const std::string_view t) -> void {
    m_placeholder = t;
}

auto neko::widget::TextInput::has_selection() const -> bool {
    return m_sel_start >= 0 && m_sel_start != m_cursor_pos;
}

