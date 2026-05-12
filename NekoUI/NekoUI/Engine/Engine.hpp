#pragma once
#include <memory>

#include "../Widget/Widget.hpp"

#include "Backend/Backend.hpp"
#include "Window/Window.hpp"

#include "parallel_hashmap/phmap.h"

namespace neko::engine {
    class Engine {
    public:
        using Handle = void*;

        template<typename B> requires std::is_base_of_v<backend::Backend, B>
        auto set_backend(std::shared_ptr<B> backend) -> bool;

        template<typename W> requires std::is_base_of_v<window::Window, W>
        auto set_window(std::shared_ptr<W> window) -> bool;

        auto remove_window(Handle handle) -> bool;

        [[nodiscard]] auto is_ready() const -> bool;
    private:
        struct ChildWindow {
            std::shared_ptr<backend::Backend> backend;
            std::shared_ptr<window::Window> window;
            widget::Widget widget;
        };

        auto msg_callback(Handle handle, window::InputType input_type, window::InputMsgType msg_type, window::InputMsg msg) -> window::MsgResult;

        phmap::flat_hash_map<Handle, ChildWindow> backend_windows;
    };
} // namespace neko::engine
