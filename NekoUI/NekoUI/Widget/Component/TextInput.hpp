// 2026-07-02 08:06:31

#pragma once

#include "../Widget.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace neko::widget {
    class TextInput final : public Widget {
    public:
        explicit TextInput(const glm::ivec4 bounds = {}, std::string placeholder = "") : m_placeholder(std::move(placeholder)) {
            set_bounds(bounds);
        }

        auto update(engine::Context& context) -> void override {
            const auto now = std::chrono::steady_clock::now();
            if (now - m_cursor_tick > std::chrono::milliseconds(500)) {
                m_cursor_visible = !m_cursor_visible;
                m_cursor_tick = now;
                context.dirty = true;
            }
            Widget::update(context);
        }

        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            const auto& b = bounds();

            // 背景 + 边框
            backend.draw_rect_fill(b, m_bg_color);
            backend.draw_rect(b, has_focus() ? m_focus_border_color : m_border_color, 1);

            // 选中背景
            if (has_focus() && m_sel_start >= 0 && m_sel_start != m_cursor_pos) {
                const int sel_begin = (std::min)(m_sel_start, m_cursor_pos);
                const int sel_end = (std::max)(m_sel_start, m_cursor_pos);
                backend.draw_rect_fill({b.x + 2, b.y + 2, (sel_end - sel_begin) * 8, b.w - 4}, m_sel_color);
            }

            // 文字（垂直居中）
            const bool empty = m_text.empty();
            const auto display_text = empty ? m_placeholder : m_text;
            const auto text_color = empty ? m_placeholder_color : m_text_color;
            constexpr int font_size = 16;
            constexpr int font_ascent = 13;  // stb_truetype ascent ≈ 0.8 * font_size
            const int text_y = b.y + (b.w - font_size) / 2 + font_ascent;
            backend.draw_text(display_text, {b.x + 4, text_y}, text_color);

            // 光标
            if (has_focus() && m_cursor_visible) {
                int cx = b.x + 4 + (m_cursor_pos * 8);
                backend.draw_rect({cx, b.y + 2, 2, b.w - 4}, m_cursor_color, 1);
            }

            Widget::draw(context, backend);
        }

        auto handle_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool override {
            if (msg == WM_CHAR) {
                const auto ch = static_cast<char>(wparam);
                if (ch >= 32 && ch <= 126) {
                    m_text.insert(m_cursor_pos, 1, ch);
                    ++m_cursor_pos;
                    m_sel_start = -1;
                    context.dirty = true;
                    if (on_text_changed) {
                        on_text_changed(m_text);
                    }
                }
                return true;
            }

            if (msg == WM_KEYDOWN) {
                switch (wparam) {
                    case VK_BACK:
                        if (has_selection()) {
                            delete_selection();
                            m_sel_start = -1;
                        } else if (m_cursor_pos > 0 && !m_text.empty()) {
                            m_text.erase(m_cursor_pos - 1, 1);
                            --m_cursor_pos;
                            m_sel_start = -1;
                        } else {
                            return true;
                        }
                        m_cursor_visible = true;
                        m_cursor_tick = std::chrono::steady_clock::now();
                        context.dirty = true;
                        if (on_text_changed) {
                            on_text_changed(m_text);
                        }
                        return true;
                    case VK_DELETE:
                        if (has_selection()) {
                            delete_selection();
                            m_sel_start = -1;
                        } else if (std::cmp_less(m_cursor_pos, m_text.size())) {
                            m_text.erase(m_cursor_pos, 1);
                            m_sel_start = -1;
                        } else {
                            return true;
                        }
                        m_cursor_visible = true;
                        m_cursor_tick = std::chrono::steady_clock::now();
                        context.dirty = true;
                        if (on_text_changed) {
                            on_text_changed(m_text);
                        }
                        return true;
                    case VK_LEFT:
                        if (m_cursor_pos > 0) {
                            --m_cursor_pos;
                            m_sel_start = -1;
                            context.dirty = true;
                        }
                        return true;
                    case VK_RIGHT:
                        if (std::cmp_less(m_cursor_pos, m_text.size())) {
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
                        m_cursor_pos = static_cast<int>(m_text.size());
                        m_sel_start = -1;
                        context.dirty = true;
                        return true;
                    case 'A':
                        if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
                            m_sel_start = 0;
                            m_cursor_pos = static_cast<int>(m_text.size());
                            context.dirty = true;
                            return true;
                        }
                        break;
                    default: ;
                }
                return true;
            }

            return Widget::handle_event(context, msg, wparam, lparam);
        }

        [[nodiscard]] auto focusable() const -> bool override {
            return true;
        }

        auto on_focus_gained() -> void override {
            m_cursor_visible = true;
            m_cursor_tick = std::chrono::steady_clock::now();
        }

        auto on_focus_lost() -> void override {
            m_cursor_visible = false;
            m_sel_start = -1;
        }

        [[nodiscard]] auto text() const -> const std::string& {
            return m_text;
        }

        auto set_text(const std::string_view t) -> void {
            m_text = t;
            m_cursor_pos = static_cast<int>(m_text.size());
            m_sel_start = -1;
        }

        auto set_placeholder(const std::string_view t) -> void {
            m_placeholder = t;
        }

        std::function<void(std::string_view)> on_text_changed;
    private:
        [[nodiscard]] auto has_selection() const -> bool {
            return m_sel_start >= 0 && m_sel_start != m_cursor_pos;
        }

        auto delete_selection() -> void {
            const int start = (std::min)(m_sel_start, m_cursor_pos);
            const int end = (std::max)(m_sel_start, m_cursor_pos);
            m_text.erase(start, end - start);
            m_cursor_pos = start;
        }

        std::string m_text;
        std::string m_placeholder;
        int m_cursor_pos = 0;
        int m_sel_start = -1;
        std::chrono::steady_clock::time_point m_cursor_tick;
        bool m_cursor_visible = true;

        type::Color m_bg_color{240, 240, 245, 255};
        type::Color m_text_color{30, 30, 30, 255};
        type::Color m_cursor_color{60, 60, 60, 255};
        type::Color m_sel_color{180, 200, 240, 255};
        type::Color m_border_color{180, 180, 190, 255};
        type::Color m_focus_border_color{60, 120, 220, 255};
        type::Color m_placeholder_color{180, 180, 180, 255};
    };
} // namespace neko::widget
