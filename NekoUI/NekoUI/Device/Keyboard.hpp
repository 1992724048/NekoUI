#pragma once
#include <algorithm>
#include <array>

namespace neko::device {
    inline constexpr int kVkControl = 0x11;
    inline constexpr int kVkShift = 0x10;
    inline constexpr int kVkMenu = 0x12;

    struct KeyEvent {
        int key;
        bool pressed;
    };

    struct CharEvent {
        wchar_t ch;
    };

    struct Keyboard {
    private:
        std::array<bool, 256> down{};
        std::array<bool, 256> prev_down{};
        std::array<wchar_t, 16> chars{};
        int char_count = 0;
    public:
        [[nodiscard]] auto ctrl() const -> bool {
            return down[kVkControl];
        }

        [[nodiscard]] auto shift() const -> bool {
            return down[kVkShift];
        }

        [[nodiscard]] auto alt() const -> bool {
            return down[kVkMenu];
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

        auto handle(const KeyEvent& e) -> void {
            prev_down = down;
            down[e.key & 0xFF] = e.pressed;
        }

        auto handle(const CharEvent& e) -> void {
            if (char_count < 16) {
                chars[char_count++] = e.ch;
            }
        }
    };
}
