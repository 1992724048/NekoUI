// 2026-08-10

#include "TextFieldInput.hpp"

#include <cstdint>
#include <string_view>

#include "../../Device/Keyboard.hpp"
#include "../../Platform/Event.hpp"
#include "../Widget.hpp"

namespace {
    // wchar_t（UTF-16 单码元，BMP 内）→ UTF-8 编码追加
    auto append_utf8(std::string& out, const wchar_t ch) -> void {
        const auto c = static_cast<uint32_t>(ch);
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }

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

    // 码点总数
    [[nodiscard]] auto count_cps(const std::string& text) -> size_t {
        size_t count = 0;
        for (const unsigned char c : text) {
            if (is_lead_byte(c)) {
                ++count;
            }
        }
        return count;
    }

    // 近似文本宽度（ASCII = font_size/2，非 ASCII = font_size——与 TextFieldDraw 同算法）
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

    // 在码点位置插入 UTF-8 字符
    auto insert_cp(std::string& text, const size_t cp, const std::string_view utf8_char) -> void {
        text.insert(cp_to_byte(text, cp), utf8_char);
    }

    // 删除指定码点
    auto erase_cp(std::string& text, const size_t cp) -> void {
        const auto byte = cp_to_byte(text, cp);
        if (byte >= text.size()) {
            return;
        }
        size_t end = byte + 1;
        while (end < text.size() && !is_lead_byte(static_cast<unsigned char>(text[end]))) {
            ++end;
        }
        text.erase(byte, end - byte);
    }

    // wchar 串 → UTF-8（BMP 内，逐码元转换）
    [[nodiscard]] auto wstring_to_utf8(const std::wstring& input) -> std::string {
        std::string out;
        out.reserve(input.size());
        for (const wchar_t ch : input) {
            append_utf8(out, ch);
        }
        return out;
    }
} // namespace

namespace neko::behavior {
    TextFieldInput::TextFieldInput(neko::widget::Widget& owner, behavior::TextFieldState& state, const behavior::GeometryState& geometry, const style::TextFieldStyle& style, const engine::Context& /*context*/) :
        InputBehavior{owner},
        state_{state},
        geometry_{geometry},
        style_{style} {}

    auto TextFieldInput::input(engine::Context& context, const platform::Event& event) -> void {
        if (const auto* char_evt = std::get_if<device::CharEvent>(&event)) {
            handle_char(context, char_evt->ch);
        } else if (const auto* key_evt = std::get_if<device::KeyEvent>(&event)) {
            handle_key(context, *key_evt);
        } else if (const auto* ime_evt = std::get_if<platform::ImeCompositionEvent>(&event)) {
            handle_ime(context, *ime_evt);
        }
    }

    auto TextFieldInput::handle_char(engine::Context& context, const wchar_t ch) -> void {
        if (ch < 0x20) {
            return; // 控制字符（退格/回车等）不经字符路径
        }
        std::string utf8;
        append_utf8(utf8, ch);
        insert_cp(state_.text, state_.caret_pos, utf8);
        ++state_.caret_pos;
        context.mark_dirty();
    }

    auto TextFieldInput::handle_key(engine::Context& context, const device::KeyEvent& key) -> void {
        if (!key.pressed) {
            return;
        }
        switch (key.key) {
        case 0x08: // VK_BACK
            if (state_.caret_pos > 0) {
                erase_cp(state_.text, state_.caret_pos - 1);
                --state_.caret_pos;
                context.mark_dirty();
            }
            break;
        case 0x25: // VK_LEFT
            if (state_.caret_pos > 0) {
                --state_.caret_pos;
                context.mark_dirty();
            }
            break;
        case 0x27: // VK_RIGHT
            if (state_.caret_pos < count_cps(state_.text)) {
                ++state_.caret_pos;
                context.mark_dirty();
            }
            break;
        default:
            break;
        }
    }

    auto TextFieldInput::handle_ime(engine::Context& context, const platform::ImeCompositionEvent& ime) -> void {
        if (ime.composition.empty()) {
            state_.ime_comp.clear();
            state_.ime_active = false;
        } else {
            state_.ime_comp = wstring_to_utf8(ime.composition);
            state_.ime_active = true;
            // IME 合成窗/候选窗定位到光标（与 TextFieldDraw 相同的近似宽度算法）
            if (context.set_ime_pos) {
                const auto bounds = geometry_.bounds;
                const auto byte_offset = cp_to_byte(state_.text, state_.caret_pos);
                const auto before = std::string_view(state_.text).substr(0, byte_offset);
                constexpr int PADDING = 4;
                const auto caret_x = bounds.x + PADDING + static_cast<int>(approx_width(before, style_.text.font_size));
                context.set_ime_pos(caret_x, bounds.y + PADDING);
            }
        }
        context.mark_dirty();
    }
} // namespace neko::behavior
