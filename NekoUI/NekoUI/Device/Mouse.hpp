// 2026-07-15 16:28:46

#pragma once
#include <Windows.h>
#include <algorithm>
#include <array>
#include <span>
#include "../Type.hpp"

namespace neko::device {
    using namespace neko::type;

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
        auto set_dpi(const UINT dpi) -> void {
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
            const float s = dpi_scale_;
            return pos.x >= r.x * s && pos.x <= (r.x + r.z) * s && pos.y >= r.y * s && pos.y <= (r.y + r.w) * s;
        }

        [[nodiscard]] auto is_inside_circle(const Vec2I center, const int radius) const -> bool {
            const float s = dpi_scale_;
            const float dx = static_cast<float>(pos.x) - (static_cast<float>(center.x) * s);
            const float dy = static_cast<float>(pos.y) - (static_cast<float>(center.y) * s);
            const float r = static_cast<float>(radius) * s;
            return (dx * dx) + (dy * dy) <= r * r;
        }

        [[nodiscard]] auto is_inside_rounded(const Vec4I r, const int corner_radius) const -> bool {
            const float s = dpi_scale_;
            const int rx = static_cast<int>(r.x * s);
            const int ry = static_cast<int>(r.y * s);
            const int rw = static_cast<int>(r.z * s);
            const int rh = static_cast<int>(r.w * s);
            const int cr = static_cast<int>(static_cast<float>(corner_radius) * s);

            if (pos.x < rx || pos.x > rx + rw || pos.y < ry || pos.y > ry + rh) {
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
            const float s = dpi_scale_;
            bool inside = false;
            for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
                const int xi = static_cast<int>(static_cast<float>(pts[i].x) * s);
                const int yi = static_cast<int>(static_cast<float>(pts[i].y) * s);
                const int xj = static_cast<int>(static_cast<float>(pts[j].x) * s);
                const int yj = static_cast<int>(static_cast<float>(pts[j].y) * s);

                if (yi > pos.y != yj > pos.y && pos.x < (static_cast<float>(xj - xi) * static_cast<float>(pos.y - yi) / static_cast<float>(yj - yi)) + xi) {
                    inside = !inside;
                }
            }
            return inside;
        }

        auto handle(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
            prev_pos = pos;
            prev_left = left_down;
            prev_right = right_down;
            prev_middle = middle_down;

            switch (msg) {
                case WM_MOUSEMOVE:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    return true;
                case WM_LBUTTONDOWN:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    left_down = true;
                    return true;
                case WM_LBUTTONUP:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    left_down = false;
                    return true;
                case WM_RBUTTONDOWN:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    right_down = true;
                    return true;
                case WM_RBUTTONUP:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    right_down = false;
                    return true;
                case WM_MBUTTONDOWN:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    middle_down = true;
                    return true;
                case WM_MBUTTONUP:
                    pos = {.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam))};
                    middle_down = false;
                    return true;
                case WM_MOUSEWHEEL:
                    wheel_delta += GET_WHEEL_DELTA_WPARAM(wparam);
                    return true;
                default:
                    return false;
            }
        }
    };
} // namespace neko::device
