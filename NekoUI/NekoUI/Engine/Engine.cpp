#include "Engine.hpp"

#include "../Widget/Component/Animation.hpp"

namespace neko::engine {
    Engine::Engine(const HWND hwnd) : backend(hwnd) {
        context.rebuild = std::bind(&Engine::rebuild, this);
        context.rerender = std::bind(&std::condition_variable::notify_one, &render_notify);
        context.animation_start = std::bind(&Engine::anim_inc, this);
        context.animation_end = std::bind(&Engine::anim_dec, this);

        render_thread_ = std::jthread(std::bind(&Engine::render_thread, this));
    }

    Engine::~Engine() {
        if (render_thread_.joinable()) {
            render_thread_.request_stop();
            render_notify.notify_one();
            render_thread_.join();
        }
    }

    auto Engine::set_widget_tree(std::shared_ptr<widget::Widget> tree) -> void {
        widget = std::move(tree);
        rebuild();
    }

    auto Engine::rebuild() -> void {
        render_notify.notify_one();
    }

    auto Engine::render_thread() -> void {
        while (!render_thread_.get_stop_token().stop_requested()) {
            if (animation == 0) {
                std::unique_lock lock(render_lock);
                render_notify.wait(lock);
            }

            if (render_thread_.get_stop_token().stop_requested()) {
                break;
            }

            animation::new_frame();

            backend.begin();

            if (widget) {
                widget->draw(context, backend);
            }

            backend.end();
        }
    }

    auto Engine::anim_inc() -> void {
        ++animation;
    }

    auto Engine::anim_dec() -> void {
        --animation;
    }
} // namespace neko::engine
