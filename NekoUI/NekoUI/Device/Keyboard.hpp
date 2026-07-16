#pragma once
#include <Windows.h>
#include <algorithm>
#include <array>

namespace neko::device {
    struct Keyboard {
    private:
        std::array<bool, 256> down{};
        std::array<bool, 256> prev_down{};
        std::array<wchar_t, 16> chars{};
        int char_count = 0;
    public:
        [[nodiscard]] auto ctrl() const -> bool {
            return down[VK_CONTROL];
        }

        [[nodiscard]] auto shift() const -> bool {
            return down[VK_SHIFT];
        }

        [[nodiscard]] auto alt() const -> bool {
            return down[VK_MENU];
        }

        [[nodiscard]] auto just_pressed(const int vk) const -> bool {
            return down[vk] && !prev_down[vk];
        }

        [[nodiscard]] auto just_released(const int vk) const -> bool {
            return !down[vk] && prev_down[vk];
        }

        [[nodiscard]] auto any_down() const -> bool {
            return std::ranges::any_of(down,
                                       [](const auto k)-> bool {
                                           return k;
                                       });
        }

        auto handle(const UINT msg, const WPARAM wparam, LPARAM lparam) -> bool {
            switch (msg) {
                case WM_KEYDOWN:
                case WM_SYSKEYDOWN:
                    prev_down = down;
                    down[wparam & 0xFF] = true;
                    return true;
                case WM_KEYUP:
                case WM_SYSKEYUP:
                    prev_down = down;
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
