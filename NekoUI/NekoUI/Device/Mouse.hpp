// 2026-07-15 16:28:46

#pragma once
#include <algorithm>
#include <array>
#include <span>

#include "../Type.hpp"

namespace neko::device {
    using namespace neko::type;

    enum class MouseButton : std::uint8_t { Left, Right, Middle };

    struct MouseMoveEvent {
        int x;
        int y;
    };

    struct MouseButtonEvent {
        int x;
        int y;
        MouseButton button;
        bool pressed;
    };

    struct MouseWheelEvent {
        int delta;
    };

    struct Mouse {
    private:
        Vec2I pos{};
        Vec2I prev_pos{};
        bool left_down = false;
        bool right_down = false;
        bool middle_down = false;
        bool prev_left = false;
        bool prev_right = false;
        bool prev_middle = false;
        int wheel_delta = 0;
        float dpi_scale_ = 1.0F;
    public:
        auto set_dpi(const uint32_t dpi) -> void {
            dpi_scale_ = dpi / 96.0F;
        }

        [[nodiscard]] auto left_clicked() const -> bool {
            return left_down && !prev_left;
        }

        [[nodiscard]] auto left_released() const -> bool {
            return !left_down && prev_left;
        }

        [[nodiscard]] auto right_clicked() const -> bool {
            return right_down && !prev_right;
        }

        [[nodiscard]] auto middle_clicked() const -> bool {
            return middle_down && !prev_middle;
        }

        [[nodiscard]] auto moved() const -> bool {
            return pos != prev_pos;
        }

        [[nodiscard]] auto is_inside(const Vec4I r) const -> bool {
            return pos.x >= r.x && pos.x < r.x + r.z && pos.y >= r.y && pos.y < r.y + r.w;
        }

        [[nodiscard]] auto is_inside_circle(const Vec2I center, const int radius) const -> bool {
            const float dx = static_cast<float>(pos.x - center.x);
            const float dy = static_cast<float>(pos.y - center.y);
            const float r = static_cast<float>(radius);
            return (dx * dx) + (dy * dy) <= r * r;
        }

        [[nodiscard]] auto is_inside_rounded(const Vec4I r, const int corner_radius) const -> bool {
            const int rx = r.x;
            const int ry = r.y;
            const int rw = r.z;
            const int rh = r.w;
            const int cr = corner_radius;

            if (pos.x < rx || pos.x >= rx + rw || pos.y < ry || pos.y >= ry + rh) {
                return false;
            }

            if (pos.x >= rx + cr && pos.x <= rx + rw - cr && pos.y >= ry + cr && pos.y <= ry + rh - cr) {
                return true;
            }

            const std::array<Vec2I, 4> corners{Vec2I{.x = rx + cr, .y = ry + cr}, {.x = rx + rw - cr, .y = ry + cr}, {.x = rx + cr, .y = ry + rh - cr}, {.x = rx + rw - cr, .y = ry + rh - cr}};
            return std::ranges::any_of(corners,
                                       [&](const Vec2I c) -> bool {
                                           const auto dx = static_cast<float>(pos.x - c.x);
                                           const auto dy = static_cast<float>(pos.y - c.y);
                                           return (dx * dx) + (dy * dy) <= static_cast<float>(cr * cr);
                                       });
        }

        [[nodiscard]] auto is_inside_polygon(const std::span<const Vec2I> pts) const -> bool {
            if (pts.size() < 3) {
                return false;
            }
            bool inside = false;
            for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
                const int xi = pts[i].x;
                const int yi = pts[i].y;
                const int xj = pts[j].x;
                const int yj = pts[j].y;

                if (yi > pos.y != yj > pos.y && pos.x < (static_cast<float>(xj - xi) * static_cast<float>(pos.y - yi) / static_cast<float>(yj - yi)) + xi) {
                    inside = !inside;
                }
            }
            return inside;
        }

        auto handle(const MouseMoveEvent& e) -> void {
            prev_pos = pos;
            pos = {.x = e.x, .y = e.y};
        }

        auto handle(const MouseButtonEvent& e) -> void {
            prev_pos = pos;
            pos = {.x = e.x, .y = e.y};
            switch (e.button) {
                case MouseButton::Left:
                    prev_left = left_down;
                    left_down = e.pressed;
                    break;
                case MouseButton::Right:
                    prev_right = right_down;
                    right_down = e.pressed;
                    break;
                case MouseButton::Middle:
                    prev_middle = middle_down;
                    middle_down = e.pressed;
                    break;
            }
        }

        auto handle(const MouseWheelEvent& e) -> void {
            wheel_delta += e.delta;
        }
    };
}
