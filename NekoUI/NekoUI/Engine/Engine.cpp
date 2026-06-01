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

    auto Engine::msg_callback(const Handle handle, window::InputState& state) -> window::MsgResult {
        std::shared_lock read_lock(mutex);
        if (handle == nullptr || !backend_windows.contains(handle)) {
            return window::MsgResult::Ignore;
        }

        auto& ins = backend_windows[handle];
        if (state.window.destroy) {
            read_lock.unlock();
            if (ins.render_thread.request_stop() && ins.render_thread.joinable()) {
                ins.notification.notify_one();
                ins.render_thread.join();
                remove(handle);
            }
            return window::MsgResult::Dispose;
        }
        if (!state.window.first_created && !ins.init) {
            ins.notification.notify_one();
        }
        if (state.window.first_created) {
            ins.render_thread = std::jthread(render_thread, std::ref(ins));
            state.window.first_created = false;
        }
        if (state.window.resized) {
            ins.dirty = true;
            ins.notification.notify_one();
            state.window.resized = false;
        }

        return window::MsgResult::Ignore;
    }

    auto Engine::render_thread(const std::stop_token& token, ChildWindow& window) -> void {
        while (!token.stop_requested()) {
            if (window.animation == 0 && !window.dirty) {
                std::unique_lock lock(window.lock);
                window.notification.wait(lock);
            }

            if (token.stop_requested()) {
                break;
            }

            window.backend->resize(window.window->get_size());
            ui_process(window);
            window.backend->submit();

            window.init = true;
            window.dirty = false;
        }
    }

    auto Engine::ui_process(ChildWindow& window) -> void {}
} // namespace neko::engine
