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

    auto Engine::set_widget_tree(const Handle handle, const std::shared_ptr<Widget>& widget_tree) -> bool {
        if (handle == nullptr || !backend_windows.contains(handle)) {
            return false;
        }

        auto& ins = backend_windows[handle];
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
            child.render_notify.notify_one();
            state.window.resized = false;
        }

        return msg_process(child, state);
    }

    auto Engine::render_thread(const std::stop_token& token, ChildWindow& window) -> void {
        while (!token.stop_requested()) {
            if (window.animation == 0) {
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
        const auto widget = window.widget_tree.load();
        if (widget && widget->dirty()) {
            widget->draw(window.backend);
        }
    }

    auto Engine::msg_process(ChildWindow& window, InputState& state) -> MsgResult {
        return MsgResult::Ignore;
    }

    auto Engine::rebuild(ChildWindow& window) -> void {
        const auto tree = window.widget_tree.load();
        if (tree == nullptr) {
            return;
        }

        const auto& childs = tree->children();
        std::unique_lock _(window.stack_lock);
        window.widget_stack = childs;

        std::unique_lock _(window.keys_lock);
        window.key2widget.clear();
        for (const auto& child : childs) {
            window.key2widget[child->id()] = child;
        }

        window.render_notify.notify_one();
    }

    auto Engine::animation_count(ChildWindow& window, const std::uint16_t num) -> void {
        window.animation += num;
    }

    auto Engine::rerender(ChildWindow& window) -> void {
        window.render_notify.notify_one();
    }
} // namespace neko::engine
