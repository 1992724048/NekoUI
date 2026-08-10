#pragma once
#include <cstdint>
#include <string>
#include <variant>

#include "../Type.hpp"
#include "../Device/Keyboard.hpp"
#include "../Device/Mouse.hpp"

namespace neko::platform {
    struct ResizeEvent {
        int width;
        int height;
    };

    struct DpiChangeEvent {
        uint32_t dpi;
    };

    enum class ThemeMode : uint8_t { Light, Dark, };

    struct ThemeChangedEvent {
        ThemeMode mode;
        type::Color color;
    };

    struct DestroyEvent {};

    // IME 合成事件（WM_IME_STARTCOMPOSITION/COMPOSITION/ENDCOMPOSITION 翻译）
    struct ImeCompositionEvent {
        std::wstring composition;  // 当前合成串（空 = 合成开始/结束）
        int cursor_pos{0};         // 合成串内光标位置（GCS_CURSORPOS）
    };

    using Event = std::variant<device::MouseMoveEvent, device::MouseButtonEvent, device::MouseWheelEvent, device::KeyEvent, device::CharEvent, ResizeEvent, DpiChangeEvent, ThemeChangedEvent, DestroyEvent, ImeCompositionEvent>;

    template<typename... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template<typename... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;
}
