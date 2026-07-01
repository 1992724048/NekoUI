#pragma once
#include <Windows.h>
#include <glm/glm.hpp>

namespace neko::mouse {
    //! @brief 鼠标状态组件
    struct Mouse {
        glm::ivec2 pos{}; // 当前鼠标位置（客户区坐标）
        bool left_down = false;
        bool right_down = false;
        bool middle_down = false;
        int wheel_delta = 0; // 累计滚轮增量

        //! @brief 更新鼠标状态，返回 true 若该消息是鼠标事件
        auto handle(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
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
