// 2026-07-02 03:02:19

#pragma once
#include <Windows.h>
#include <span>
#include <glm/glm.hpp>

namespace neko::mouse {
    struct Mouse {
        glm::ivec2 pos{};
        glm::ivec2 prev_pos{};
        bool left_down = false;
        bool right_down = false;
        bool middle_down = false;
        bool prev_left = false;
        bool prev_right = false;
        bool prev_middle = false;
        int wheel_delta = 0;

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

        [[nodiscard]] auto is_inside(const glm::ivec4 r, const float scale = 1.0F) const -> bool {
            return pos.x >= r.x * scale && pos.x <= (r.x + r.z) * scale && pos.y >= r.y * scale && pos.y <= (r.y + r.w) * scale;
        }

        [[nodiscard]] auto is_inside_circle(const glm::ivec2 center, const int radius, const float scale = 1.0F) const -> bool {
            const float dx = static_cast<float>(pos.x) - (static_cast<float>(center.x) * scale);
            const float dy = static_cast<float>(pos.y) - (static_cast<float>(center.y) * scale);
            const float r = static_cast<float>(radius) * scale;
            return (dx * dx) + (dy * dy) <= r * r;
        }

        [[nodiscard]] auto is_inside_rounded(const glm::ivec4 r, const int corner_radius, const float scale = 1.0F) const -> bool {
            const int rx = static_cast<int>(r.x * scale);
            const int ry = static_cast<int>(r.y * scale);
            const int rw = static_cast<int>(r.z * scale);
            const int rh = static_cast<int>(r.w * scale);
            const int cr = static_cast<int>(corner_radius * scale);

            if (pos.x < rx || pos.x > rx + rw || pos.y < ry || pos.y > ry + rh) {
                return false;
            }

            if (pos.x >= rx + cr && pos.x <= rx + rw - cr && pos.y >= ry + cr && pos.y <= ry + rh - cr) {
                return true;
            }

            const std::array<glm::ivec2, 4> corners{glm::ivec2{rx + cr, ry + cr}, {rx + rw - cr, ry + cr}, {rx + cr, ry + rh - cr}, {rx + rw - cr, ry + rh - cr}};
            return std::ranges::any_of(corners,
                                       [&](const glm::ivec2 c) -> bool {
                                           const auto dx = static_cast<float>(pos.x - c.x);
                                           const auto dy = static_cast<float>(pos.y - c.y);
                                           return (dx * dx) + (dy * dy) <= static_cast<float>(cr * cr);
                                       });
        }

        [[nodiscard]] auto is_inside_polygon(const std::span<const glm::ivec2> pts, const float scale = 1.0F) const -> bool {
            if (pts.size() < 3) {
                return false;
            }
            bool inside = false;
            for (size_t i = 0, j = pts.size() - 1; i < pts.size(); j = i++) {
                const int xi = static_cast<int>(pts[i].x * scale);
                const int yi = static_cast<int>(pts[i].y * scale);
                const int xj = static_cast<int>(pts[j].x * scale);
                const int yj = static_cast<int>(pts[j].y * scale);

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
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    return true;
                case WM_LBUTTONDOWN:
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    left_down = true;
                    return true;
                case WM_LBUTTONUP:
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    left_down = false;
                    return true;
                case WM_RBUTTONDOWN:
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    right_down = true;
                    return true;
                case WM_RBUTTONUP:
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    right_down = false;
                    return true;
                case WM_MBUTTONDOWN:
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    middle_down = true;
                    return true;
                case WM_MBUTTONUP:
                    pos = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
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
} // namespace neko::mouse
