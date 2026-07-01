#pragma once
#include <Windows.h>
#include <array>

namespace neko::keyboard {
    //! @brief 键盘状态组件
    struct Keyboard {
        std::array<bool, 256> down{};
        std::array<wchar_t, 16> chars{};
        int char_count = 0;

        [[nodiscard]] auto ctrl() const -> bool {
            return down[VK_CONTROL];
        }

        [[nodiscard]] auto shift() const -> bool {
            return down[VK_SHIFT];
        }

        [[nodiscard]] auto alt() const -> bool {
            return down[VK_MENU];
        }

        //! @brief 更新键盘状态，返回 true 若该消息是键盘事件
        auto handle(const UINT msg, const WPARAM wparam, LPARAM lparam) -> bool {
            switch (msg) {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                    down[wparam & 0xFF] = true;
                    return true;
                case WM_KEYUP:
                case WM_SYSKEYUP:
                    down[wparam & 0xFF] = false;
                    return true;
                case WM_CHAR:
                    if (char_count < 16) {
                        chars[char_count++] = static_cast<wchar_t>(wparam);
                    }
                    return true;
                default:
                    return false;
            }
        }
    };
} // namespace neko::keyboard
