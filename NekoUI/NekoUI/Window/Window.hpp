#pragma once
#include <array>
#include <functional>

#include "../Type.hpp"

namespace neko::window {
    using namespace neko::type;

    enum class MsgResult { Dispose, Ignore };

    enum class MouseMsgType { Down, Up, Wheel };

    enum class KeyboardMsgType { Down, Up, };

    enum class MouseMsgEnum {};

    enum class KeyboardMsgEnum {};

    struct MouseMsg {
        MouseMsgEnum key;
        int wheel;
    };

    struct KeyboardMsg {
        KeyboardMsgEnum key;
        std::array<KeyboardMsgEnum, 6> modify;
    };

    enum InputType { Mouse, Keyboard };

    union InputMsgType {
        MouseMsgType mouse;
        KeyboardMsgType keyboard;
    };

    union InputMsg {
        MouseMsg mouse;
        KeyboardMsg keyboard;
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

        std::function<MsgResult(InputType, InputMsgType, InputMsg)> msg_callback;
    };
} // namespace neko::window
