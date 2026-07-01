// 2026-07-02 00:23:43

#include "Engine.hpp"

#include "../Widget/Component/Animation.hpp"

namespace neko::engine {
    Engine::Engine(const HWND hwnd) : backend(hwnd) {
        context.rebuild = std::bind(&Engine::rebuild, this);
        context.rerender = std::bind(&std::condition_variable::notify_one, &render_notify);
        context.animation_start = std::bind(&Engine::anim_inc, this);
        context.animation_end = std::bind(&Engine::anim_dec, this);

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

    auto Engine::set_widget(std::shared_ptr<widget::Widget> widget) -> void {
        this->widget = std::move(widget);
        rebuild();
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
            if (animation == 0) {
                std::unique_lock lock(render_lock);
                render_notify.wait(lock,
                                   [this] -> bool {
                                       return render_thread.get_stop_token().stop_requested() || pending;
                                   });
                pending = false;
            }
            if (render_thread.get_stop_token().stop_requested()) {
                break;
            }

            if (resize_pending.exchange(false)) {
                backend.resize(resize_size);
            }

            animation::new_frame();

            backend.begin();
            if (widget) {
                widget->layout({0, 0, resize_size});
                widget->draw(context, backend);
            }
            backend.end();
        }
    }

    auto Engine::msg_loop() -> void {
        while (!msg_thread.get_stop_token().stop_requested()) {
            MsgEvent ev;
            {
                std::unique_lock lock(msg_mutex);
                msg_notify.wait(lock,
                                [this] -> bool {
                                    return msg_thread.get_stop_token().stop_requested() || msg_count > 0;
                                });
                if (msg_thread.get_stop_token().stop_requested()) {
                    break;
                }

                ev = msg_queue[msg_head];
                msg_head = (msg_head + 1) % MSG_QUEUE_MAX;
                --msg_count;
            }
            msg_space.notify_one();

            const auto [msg, wparam, lparam] = ev;
            context.mouse.handle(msg, wparam, lparam);
            context.keyboard.handle(msg, wparam, lparam);

            switch (msg) {
                case WM_SIZE:
                    resize_size = {static_cast<int>(LOWORD(lparam)), static_cast<int>(HIWORD(lparam))};
                    resize_pending.store(true, std::memory_order_release);
                    break;
                default:
                    if (widget) {
                        widget->handle_event(context, msg, wparam, lparam);
                    }
                    break;
            }

            if (widget) {
                widget->update(context);
            }

            rebuild();
        }
    }

    auto Engine::anim_inc() -> void {
        ++animation;
    }

    auto Engine::anim_dec() -> void {
        --animation;
    }
} // namespace neko::engine
