#include "Engine.hpp"

#include <stdexcept>

namespace neko::engine {
    template<typename B> requires std::is_base_of_v<backend::Backend, B>
    auto Engine::set_backend(std::shared_ptr<B> backend) -> bool {
        if (backend == nullptr) {
            return false;
        }
        const Handle handle = backend->get_window_handle();
        if (handle == nullptr) {
            return false;
        }
        backend_windows[handle].backend = backend;
        return true;
    }

    template<typename W> requires std::is_base_of_v<window::Window, W>
    auto Engine::set_window(std::shared_ptr<W> window) -> bool {
        if (window == nullptr) {
            return false;
        }
        const Handle handle = window->get_handle();
        if (handle == nullptr) {
            return false;
        }

        window->msg_callback = std::bind(&Engine::msg_callback, this, handle, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        backend_windows[handle].window = window;
        return true;
    }

    auto Engine::remove_window(const Handle handle) -> bool {
        return backend_windows.erase(handle) != 0U;
    }

    auto Engine::is_ready() const -> bool {
        return !backend_windows.empty();
    }

    auto Engine::msg_callback(const Handle handle, window::InputType input_type, window::InputMsgType msg_type, window::InputMsg msg) -> window::MsgResult {
        if (handle == nullptr || !backend_windows.contains(handle)) {
            return window::MsgResult::Dispose;
        }

        auto& [backend, window, widget] = backend_windows[handle];

        return window::MsgResult::Dispose;
    }
} // namespace neko::engine
