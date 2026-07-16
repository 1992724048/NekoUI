#pragma once
#include <cstdint>
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

    using Event = std::variant<device::MouseMoveEvent, device::MouseButtonEvent, device::MouseWheelEvent, device::KeyEvent, device::CharEvent, ResizeEvent, DpiChangeEvent, ThemeChangedEvent, DestroyEvent>;

    template<typename... Ts>
    struct Overloaded : Ts... {
        using Ts::operator()...;
    };

    template<typename... Ts>
    Overloaded(Ts...) -> Overloaded<Ts...>;
}
