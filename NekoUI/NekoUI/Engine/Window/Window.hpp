#pragma once
#include <array>
#include <glm/glm.hpp>

namespace neko::engine {
    class Engine;
} // namespace neko::engine

namespace neko::window {
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

    class Window {
    public:
        using Handle = void*;

        virtual ~Window() = default;

        virtual auto get_handle() -> Handle {
            return nullptr;
        }
    private:
        std::function<MsgResult(InputType, InputMsgType, InputMsg)> msg_callback;
        friend class engine::Engine;
    };
} // namespace neko::window
