// 2026-07-02 00:23:43 (updated for retained mode)

#include "Engine.hpp"

#include <cmath>

#include "Component/Animation.hpp"

namespace neko::engine {
    Engine::Engine(const HWND hwnd) {
        backend = std::make_unique<backend::Backend>(hwnd);
        context = std::make_unique<Context>();

        mouse = std::make_shared<mouse::Mouse>();
        keyboard = std::make_shared<keyboard::Keyboard>();
        scheme = std::make_shared<color::ColorScheme>();
        *scheme = scheme->from_seed({19 / 255, 161 / 255, 14 / 255, 1});

        const UINT initial_dpi = static_cast<UINT>(std::round(backend->get_dpi_scale() * 96.0F));
        mouse->set_dpi(initial_dpi);

        context->present = std::bind(&Engine::present, this);
        context->mark_dirty = std::bind(&Engine::mark_dirty, this);
        context->anim_inc = std::bind(&Engine::anim_inc, this);
        context->anim_dec = std::bind(&Engine::anim_dec, this);
        context->mouse = mouse;
        context->keyboard = keyboard;
        context->color_scheme = scheme;

        msg_thread = std::jthread(&Engine::msg_loop, this);
        render_thread = std::jthread(&Engine::render_loop, this);
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

    auto Engine::clear() -> void {
        root = nullptr;
        focused.load().reset();
    }

    auto Engine::present() -> void {
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

    auto Engine::del(widget::Widget* widget) -> bool {
        present();
        return true;
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
            std::this_thread::yield();
            return;
        }
        std::unique_lock lock(render_lock);
        render_notify.wait(lock,
                           [this] -> bool {
                               return render_thread.get_stop_token().stop_requested() || pending || dirty;
                           });
        pending = false;
    }

    auto Engine::render_frame() -> void {
        if (resize_pending.exchange(false)) {
            backend->resize(resize_size);
        }

        backend->begin(scheme->primary);
        const auto widget = root.load();
        if (widget) {
            widget->layout({.x = 0, .y = 0, .width = resize_size.x, .height = resize_size.y});
            widget->draw(*context, *backend);
        }
        backend->end();
        dirty = false;
    }

    auto Engine::msg_loop() -> void {
        while (!msg_thread.get_stop_token().stop_requested()) {
            const auto ev = msg_dequeue();
            if (!ev.has_value()) {
                break;
            }

            const auto [msg, wparam, lparam] = *ev;
            mouse->handle(msg, wparam, lparam);
            keyboard->handle(msg, wparam, lparam);
            msg_dispatch(msg, wparam, lparam);

            root.load()->update(*context);
            if (dirty) {
                present();
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
            case WM_SIZE: {
                resize_size = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                resize_pending.store(true, std::memory_order_release);
                dirty = true;
                break;
            }
            case WM_DPICHANGED: {
                const UINT dpi = LOWORD(wparam);
                backend->set_dpi(dpi);
                mouse->set_dpi(dpi);
                dirty = true;
                break;
            }
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
                if (root.load()->raw_event(*context, msg, wparam, lparam) || root.load()->hit_test(*mouse)) {
                    break;
                }
                break;
            case WM_KEYDOWN:
                if (wparam == VK_TAB) {
                    return;
                }
            case WM_KEYUP:
            case WM_CHAR:
                if (wparam == '\t') {
                    break;
                }
                if (!focused.load().expired()) {
                    focused.load().lock()->raw_event(*context, msg, wparam, lparam);
                }
                break;
            case WM_TIMER:
                break;
            default: ;
        }
    }

    auto Engine::anim_inc() -> void {
        animation.fetch_add(1, std::memory_order_relaxed);
    }

    auto Engine::anim_dec() -> void {
        if (animation.fetch_sub(1, std::memory_order_relaxed) == 0) {
            animation.store(0, std::memory_order_relaxed);
        }
    }

    auto Engine::mark_dirty() -> void {
        dirty = true;
    }
} // namespace neko::engine
