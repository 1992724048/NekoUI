// 2026-06-01 01:06:52

#pragma once
#include <bitset>
#include <functional>
#include <optional>

#include "../Type.hpp"

namespace neko::window {
    using namespace neko::type;

    //! @brief 消息处理结果
    enum class MsgResult : std::uint8_t { Dispose, Ignore };

    //! @brief 鼠标状态索引
    enum class MouseKey : std::uint8_t {
        RightButton,
        LeftButton,
        MiddleButton,
        X1Button,
        X2Button,
        WheelDown,
        WheelUp
    };

    //! @brief 键盘状态索引
    enum class KeyboardKey : std::uint8_t {
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
        I,
        J,
        K,
        L,
        M,
        N,
        O,
        P,
        Q,
        R,
        S,
        T,
        U,
        V,
        W,
        X,
        Y,
        Z
    };

    //! @brief 鼠标状态
    struct MouseState {
        std::bitset<7> keys;
        int wheel, x, y;
    };

    //! @brief 键盘状态
    struct KeyboardState {
        std::bitset<108> keys;
    };

    struct WindowState {
        //! @brief 窗口大小
        Vec2<int> window_size{.x = 0, .y = 0};

        //! @brief 大小是否改变
        bool resized{false};

        //! @brief 是否第一次创建
        bool first_create{true};
    };

    //! @brief 输入状态
    struct InputState {
        MouseState mouse;
        KeyboardState keyboard;
        WindowState window;
    };

    //! @brief 窗口对象
    class Window {
    public:
        virtual ~Window() = default;

        virtual auto get_handle() -> Handle {
            return nullptr;
        }

        [[nodiscard]] auto get_size() const -> Vec2<int> {
            return state.window.window_size;
        }

        std::function<MsgResult()> msg_callback;
        std::function<void()> destroy_callback;
        InputState state{};
    };
} // namespace neko::window
