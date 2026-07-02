// 2026-07-02 00:23:43 (updated for retained mode)

#include "Engine.hpp"

#include "../Widget/Component/Animation.hpp"

namespace neko::engine {
    Engine::Engine(const HWND hwnd) : backend(hwnd) {
        context.rebuild = std::bind(&Engine::rebuild, this);
        context.rerender = std::bind(&std::condition_variable::notify_one, &render_notify);
        context.animation_start = std::bind(&Engine::anim_inc, this);
        context.animation_end = std::bind(&Engine::anim_dec, this);
        context.request_focus = [this](widget::Widget* w) -> void { focus_widget(w); };
        context.dpi_scale = backend.get_dpi_scale();

        msg_thread = std::jthread(std::bind(&Engine::msg_loop, this));
        render_thread = std::jthread(std::bind(&Engine::render_loop, this));
    }

    Engine::~Engine() {
        if (render_thread.joinable()) {
            render_thread.request_stop();
            render_notify.notify_one();
            render_thread.join();
        }
        if (msg_thread.joinable()) {
            msg_thread.request_stop();
            msg_notify.notify_one();
            msg_thread.join();
        }
    }

    auto Engine::rebuild() -> void {
        pending = true;
        render_notify.notify_one();
    }

    auto Engine::push_msg(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> void {
        std::unique_lock lock(msg_mutex);
        msg_space.wait(lock,
                       [this] -> bool {
                           return msg_count < MSG_QUEUE_MAX;
                       });

        msg_queue[msg_tail] = MsgEvent{msg, wparam, lparam};
        msg_tail = (msg_tail + 1) % MSG_QUEUE_MAX;
        ++msg_count;

        msg_notify.notify_one();
    }

    auto Engine::render_loop() -> void {
        while (!render_thread.get_stop_token().stop_requested()) {
            render_wait();
            if (render_thread.get_stop_token().stop_requested()) {
                break;
            }
            render_frame();
        }
    }

    auto Engine::render_wait() -> void {
        if (animation != 0) {
            return;
        }
        std::unique_lock lock(render_lock);
        render_notify.wait(lock,
                           [this] -> bool {
                               return render_thread.get_stop_token().stop_requested() || pending || context.dirty;
                           });
        pending = false;
    }

    auto Engine::render_frame() -> void {
        if (resize_pending.exchange(false)) {
            backend.resize(resize_size);
        }

        backend.begin();
        for (const auto& root : m_root_widgets) {
            root->layout({0, 0, resize_size.x, resize_size.y});
            root->draw(context, backend);
        }
        backend.end();
        context.dirty = false;
    }

    auto Engine::msg_loop() -> void {
        while (!msg_thread.get_stop_token().stop_requested()) {
            const auto ev = msg_dequeue();
            if (!ev.has_value()) {
                break;
            }

            const auto [msg, wparam, lparam] = *ev;
            context.mouse.handle(msg, wparam, lparam);
            context.keyboard.handle(msg, wparam, lparam);
            msg_dispatch(msg, wparam, lparam);

            for (const auto& root : m_root_widgets) {
                root->update(context);
            }
            if (context.dirty) {
                rebuild();
            }
        }
    }

    auto Engine::msg_dequeue() -> std::optional<MsgEvent> {
        std::unique_lock lock(msg_mutex);
        msg_notify.wait(lock,
                        [this] -> bool {
                            return msg_thread.get_stop_token().stop_requested() || msg_count > 0;
                        });
        if (msg_thread.get_stop_token().stop_requested()) {
            return std::nullopt;
        }

        const auto ev = msg_queue[msg_head];
        msg_head = (msg_head + 1) % MSG_QUEUE_MAX;
        --msg_count;
        lock.unlock();
        msg_space.notify_one();
        return ev;
    }

    auto Engine::msg_dispatch(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> void {
        switch (msg) {
            case WM_SIZE:
                resize_size = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                resize_pending.store(true, std::memory_order_release);
                context.dirty = true;
                break;
            case WM_DPICHANGED: {
                const UINT dpi = LOWORD(wparam);
                backend.set_dpi(dpi);
                context.dpi_scale = static_cast<float>(dpi) / 96.0F;
                context.dirty = true;
                break;
            }
            default:
                switch (msg) {
                    case WM_MOUSEMOVE:
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                    case WM_MOUSEWHEEL:
                        for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
                            if ((*it)->handle_event(context, msg, wparam, lparam)) {
                                break;
                            }
                        }
                        break;
                    case WM_KEYDOWN:
                        if (wparam == VK_TAB) {
                            focus_next();
                            return;
                        }
                        [[fallthrough]];
                    case WM_KEYUP:
                    case WM_CHAR:
                        if (wparam == '\t') break;  // Tab 切换焦点时的残留字符
                        if (m_focused_widget) {
                            m_focused_widget->handle_event(context, msg, wparam, lparam);
                        }
                        break;
                    default:
                        for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
                            if ((*it)->handle_event(context, msg, wparam, lparam)) {
                                break;
                            }
                        }
                        break;
                }
                break;
        }
    }

    auto Engine::focus_widget(widget::Widget* w) -> void {
        if (m_focused_widget == w) return;
        if (m_focused_widget) {
            m_focused_widget->m_has_focus = false;
            m_focused_widget->on_focus_lost();
        }
        m_focused_widget = w;
        if (w) {
            w->m_has_focus = true;
            w->on_focus_gained();
        }
        context.dirty = true;
    }

    auto Engine::collect_focusable(widget::Widget* w, std::vector<widget::Widget*>& out) -> void {
        if (!w) return;
        if (w->focusable()) out.push_back(w);
        for (auto* child : w->children()) {
            collect_focusable(child, out);
        }
    }

    auto Engine::focus_next() -> void {
        std::vector<widget::Widget*> widgets;
        for (auto& root : m_root_widgets) {
            collect_focusable(root.get(), widgets);
        }
        if (widgets.empty()) {
            focus_widget(nullptr);
            return;
        }

        auto it = std::find(widgets.begin(), widgets.end(), m_focused_widget);
        size_t idx;
        if (it != widgets.end()) {
            idx = (std::distance(widgets.begin(), it) + 1) % widgets.size();
        } else {
            idx = 0;
        }
        focus_widget(widgets[idx]);
    }

    auto Engine::anim_inc() -> void {
        ++animation;
    }

    auto Engine::anim_dec() -> void {
        --animation;
    }
} // namespace neko::engine
