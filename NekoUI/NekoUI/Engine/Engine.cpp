#include "Engine.hpp"

#include <stdexcept>

namespace neko::engine {
    auto Engine::remove(const Handle handle) -> bool {
        std::unique_lock _(mutex);
        return backend_windows.erase(handle) != 0U;
    }

    auto Engine::is_ready() -> bool {
        std::shared_lock _(mutex);
        return !backend_windows.empty();
    }

    auto Engine::msg_callback(const Handle handle, window::InputType input_type, window::InputMsgType msg_type, window::InputMsg msg) -> window::MsgResult {
        std::shared_lock _(mutex);
        if (handle == nullptr || !backend_windows.contains(handle)) {
            return window::MsgResult::Ignore;
        }

        auto& ins = backend_windows[handle];

        return window::MsgResult::Dispose;
    }
} // namespace neko::engine
