// 2026-06-01 01:06:52

#pragma once
#include <bitset>
#include <functional>
#include <optional>

#include "../Type.hpp"

namespace neko::window {
    using namespace neko::type;

    /**
     * <summary>
     * 消息结果
     * </summary>
     */
    enum class MsgResult : std::uint8_t { Dispose, Ignore };

    /**
     * <summary>
     * 鼠标状态索引
     * </summary>
     */
    enum class MouseKey : std::uint8_t {
        RightButton,
        LeftButton,
        MiddleButton,
        X1Button,
        X2Button,
        WheelDown,
        WheelUp
    };

    /**
     * <summary>
     * 键盘状态索引
     * </summary>
     */
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

    /**
     * <summary>
     * 鼠标状态
     * </summary>
     */
    struct MouseState {
        std::bitset<7> keys;
        int wheel, x, y;
    };

    /**
     * <summary>
     * 键盘状态
     * </summary>
     */
    struct KeyboardState {
        std::bitset<108> keys;
    };

    struct WindowState {
        Vec2<int> window_size{.x = 0, .y = 0};
        /**
         * <summary>
         * 大小是否改变
         * </summary>
         */
        bool resized{false};
        /**
         * <summary>
         * 是否第一次创建
         * </summary>
         */
        bool first_created{true};
        /**
         * <summary>
         * 是否销毁
         * </summary>
         */
        bool destroy{false};
    };

    /**
     * <summary>
     * 输入状态
     * </summary>
     */
    struct InputState {
        MouseState mouse;
        KeyboardState keyboard;
        WindowState window;
    };

    /**
     * <summary>
     * 窗口对象
     * </summary>
     */
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
        InputState state{};
    };
} // namespace neko::window
