// 2026-08-10

#include "TextFieldDraw.hpp"

#include <chrono>
#include <string_view>

#include "../../Backend/DirectX11.hpp"
#include "TextFieldStyle.hpp"
#include "../Widget.hpp"

namespace {
    // UTF-8 前导字节（ASCII 或多字节序列首字节）
    [[nodiscard]] auto is_lead_byte(const unsigned char c) -> bool {
        return (c & 0xC0) != 0x80;
    }

    // 码点索引 → 字节偏移
    [[nodiscard]] auto cp_to_byte(const std::string& text, const size_t cp) -> size_t {
        size_t byte = 0;
        size_t seen = 0;
        while (byte < text.size() && seen < cp) {
            if (is_lead_byte(static_cast<unsigned char>(text[byte]))) {
                ++seen;
            }
            ++byte;
        }
        return byte;
    }

    // 近似文本宽度（ASCII = font_size/2，非 ASCII = font_size——MVP 近似）
    [[nodiscard]] auto approx_width(const std::string_view text, const float font_size) -> float {
        float width = 0.0F;
        for (const unsigned char c : text) {
            if (!is_lead_byte(c)) {
                continue;
            }
            width += c < 0x80 ? font_size * 0.5F : font_size;
        }
        return width;
    }
} // namespace

namespace neko::behavior {
    TextFieldDraw::TextFieldDraw(neko::widget::Widget& owner, const behavior::GeometryState& geometry, const InteractionState& interaction, const TextFieldState& state, const style::TextFieldStyle& style, const engine::Context& /*context*/) :
        DrawBehavior{owner},
        geometry_{geometry},
        interaction_{interaction},
        state_{state},
        style_{style} {}

    auto TextFieldDraw::draw(Vec4I /*rect*/, engine::Context& context, backend::DirectX11& backend) -> Rect {
        const auto bounds = geometry_.bounds;
        const bool focused = interaction_.hovered.load(std::memory_order_relaxed);

        const auto bg_color = style_.background.color.value != 0 ? style_.background.color : context.scheme.surface;
        backend.draw_rect_fill(bounds, bg_color);

        const auto caret_color = focused ? style_.caret_color_focus : style_.caret_color;
        const auto resolved_caret = caret_color.value != 0 ? caret_color : context.scheme.on_surface;

        if (style_.border.width > 0.0F) {
            // hovered 作为聚焦视觉（MVP）
            const auto border_color = focused ? resolved_caret : style_.border.color;
            backend.draw_rect(bounds, border_color, static_cast<int>(style_.border.width));
        }

        constexpr int PADDING = 4;
        const auto text_color = style_.text.color.value != 0 ? style_.text.color : context.scheme.on_surface;
        const auto font_size = style_.text.font_size;
        const auto text_y = bounds.y + PADDING;
        const auto byte_offset = cp_to_byte(state_.text, state_.caret_pos);
        const auto before = std::string_view(state_.text).substr(0, byte_offset);
        const auto after = std::string_view(state_.text).substr(byte_offset);
        const auto caret_x = bounds.x + PADDING + static_cast<int>(approx_width(before, font_size));

        backend.draw_text(before, Vec2I{.x = bounds.x + PADDING, .y = text_y}, text_color, font_size);
        backend.draw_text(after, Vec2I{.x = caret_x, .y = text_y}, text_color, font_size);

        if (!state_.ime_active && focused) {
            const auto now = std::chrono::steady_clock::now().time_since_epoch();
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
            if ((ms / 500) % 2 == 0) {
                backend.draw_line(Vec2I{.x = caret_x, .y = bounds.y + PADDING}, Vec2I{.x = caret_x, .y = bounds.w - PADDING}, resolved_caret, 2);
            }
        }

        if (!state_.ime_comp.empty()) {
            backend.draw_text(state_.ime_comp, Vec2I{.x = caret_x, .y = text_y}, style_.comp_color, font_size);
            const auto comp_width = static_cast<int>(approx_width(state_.ime_comp, font_size));
            const auto underline_y = text_y + static_cast<int>(font_size) + 1;
            backend.draw_line(Vec2I{.x = caret_x, .y = underline_y}, Vec2I{.x = caret_x + comp_width, .y = underline_y}, style_.comp_color, 1);
        }

        return {.x = bounds.x, .y = bounds.y, .width = bounds.z - bounds.x, .height = bounds.w - bounds.y};
    }
} // namespace neko::behavior
