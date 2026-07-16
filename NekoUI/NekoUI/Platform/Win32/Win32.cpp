#include "Win32.hpp"
#ifdef _WIN32
namespace neko::platform {
    NEKO_REGISTER_PLATFORM(Win32)

    auto Win32::translate_event(const NativeMessage& nm) const -> std::optional<Event> {
        const UINT msg = nm.msg;
        const WPARAM wparam = nm.wparam;
        const LPARAM lparam = nm.lparam;

        switch (msg) {
            case WM_MOUSEMOVE:
                return device::MouseMoveEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                };

            case WM_LBUTTONDOWN:
                return device::MouseButtonEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                    .button = device::MouseButton::Left,
                    .pressed = true,
                };
            case WM_LBUTTONUP:
                return device::MouseButtonEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                    .button = device::MouseButton::Left,
                    .pressed = false,
                };
            case WM_RBUTTONDOWN:
                return device::MouseButtonEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                    .button = device::MouseButton::Right,
                    .pressed = true,
                };
            case WM_RBUTTONUP:
                return device::MouseButtonEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                    .button = device::MouseButton::Right,
                    .pressed = false,
                };
            case WM_MBUTTONDOWN:
                return device::MouseButtonEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                    .button = device::MouseButton::Middle,
                    .pressed = true,
                };
            case WM_MBUTTONUP:
                return device::MouseButtonEvent{
                    .x = static_cast<int>(LOWORD(lparam)),
                    .y = static_cast<int>(HIWORD(lparam)),
                    .button = device::MouseButton::Middle,
                    .pressed = false,
                };
            case WM_MOUSEWHEEL:
                return device::MouseWheelEvent{
                    .delta = GET_WHEEL_DELTA_WPARAM(wparam),
                };

            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                return device::KeyEvent{
                    .key = static_cast<int>(wparam),
                    .pressed = true,
                };
            case WM_KEYUP:
            case WM_SYSKEYUP:
                return device::KeyEvent{
                    .key = static_cast<int>(wparam),
                    .pressed = false,
                };
            case WM_CHAR:
                return device::CharEvent{
                    .ch = static_cast<wchar_t>(wparam),
                };

            case WM_SIZE:
                return ResizeEvent{
                    .width = static_cast<int>(LOWORD(lparam)),
                    .height = static_cast<int>(HIWORD(lparam)),
                };

            case WM_DPICHANGED:
                return DpiChangeEvent{
                    .dpi = static_cast<uint32_t>(LOWORD(wparam)),
                };

            case WM_DESTROY:
                return DestroyEvent{};

            default:
                return std::nullopt;
        }
    }

    Win32::Win32() = default;
}
#endif
