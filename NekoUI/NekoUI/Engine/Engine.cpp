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

    auto Engine::destroy() -> void {
        std::unique_lock _(mutex);
        backend_windows.clear();
    }

    auto Engine::get_widget(const Handle handle, const std::string& widget_id) -> std::shared_ptr<Widget> {
        if (handle == nullptr || !backend_windows.contains(handle)) {
            return nullptr;
        }

        const auto& ins = backend_windows[handle];
        while (auto widget = ins.widget_tree.load()) {
            if (widget_id == widget->id_key) {
                return widget;
            }
            widget = widget->child.load();
        }

        return nullptr;
    }

    auto Engine::set_widget_tree(const Handle handle, const std::shared_ptr<Widget>& widget_tree) -> bool {
        if (handle == nullptr || !backend_windows.contains(handle)) {
            return false;
        }

        auto widget = widget_tree;
        while (widget != nullptr) {
            if (widget->child.load() == widget) {
                return false;
            }
            widget = widget->child.load();
        }

        auto& ins = backend_windows[handle];
        ins.dirty = true;
        ins.widget_tree = widget_tree;
        return true;
    }

    auto Engine::msg_callback(ChildWindow& child, InputState& state) -> MsgResult {
        if (state.window.destroy) {
            if (child.render_thread.request_stop() && child.render_thread.joinable()) {
                child.render_notify.notify_one();
                child.render_thread.join();
            }
            remove(child.window->get_handle());
            return MsgResult::Dispose;
        }
        if (!state.window.first_create && !child.init) {
            child.render_notify.notify_one();
        }
        if (state.window.first_create) {
            child.render_thread = std::jthread(render_thread, std::ref(child));
            state.window.first_create = false;
        }
        if (state.window.resized) {
            child.dirty = true;
            child.render_notify.notify_one();
            state.window.resized = false;
        }

        return msg_process(child);
    }

    auto Engine::render_thread(const std::stop_token& token, ChildWindow& window) -> void {
        while (!token.stop_requested()) {
            if (window.animation == 0 && !window.dirty) {
                std::unique_lock lock(window.render_lock);
                window.render_notify.wait(lock);
            }

            if (token.stop_requested()) {
                break;
            }

            window.backend->resize(window.window->get_size());
            window.backend->submit();

            window.init = true;
        }
    }

    auto Engine::render_process(ChildWindow& window) -> void {
        auto widget = window.widget_tree.load();
        while (widget != nullptr) {
            widget->draw();
            widget = widget->child.load();
        }
        window.dirty = false;
    }

    auto Engine::msg_process(ChildWindow& window) -> MsgResult {
        return MsgResult::Ignore;
    }
} // namespace neko::engine
