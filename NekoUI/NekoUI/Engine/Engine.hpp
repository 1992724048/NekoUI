#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>

#include "Context.hpp"
#include "../Type.hpp"

#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

#include "Component/Animation.hpp"

namespace neko::engine {
    using namespace neko::type;

    class Engine final {
    public:
        explicit Engine(HWND hwnd);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;

        template<typename T, typename... Args> requires std::is_base_of_v<widget::Widget, T>
        auto set_root_widget(Args&&... args) -> std::shared_ptr<T> {
            const std::shared_ptr<widget::Widget> widget = std::make_shared<T>(this, std::forward<Args>(args)...);
            root = widget;
            frame();
            return widget;
        }

        auto clear() -> void;
        auto frame() -> void;
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;
    protected:
        friend widget::Widget;

        auto del_widget(widget::Widget* widget) -> bool;
        auto reg_widget(std::weak_ptr<widget::Widget> widget);
        auto reg_animation(animation::AnimationBase& widget);
    private:
        std::unique_ptr<Context> context{};
        std::unique_ptr<backend::Backend> backend{};
        std::shared_ptr<mouse::Mouse> mouse;
        std::shared_ptr<keyboard::Keyboard> keyboard;

        std::atomic_int animation{};
        auto anim_inc() -> void;
        auto anim_dec() -> void;

        std::mutex render_lock;
        std::condition_variable render_notify;
        std::jthread render_thread;
        auto render_loop() -> void;
        auto render_wait() -> void;
        auto render_frame() -> void;

        static constexpr size_t MSG_QUEUE_MAX = 32;
        using MsgEvent = std::tuple<UINT, WPARAM, LPARAM>;
        std::array<MsgEvent, MSG_QUEUE_MAX> msg_queue{};
        std::mutex msg_mutex;
        std::condition_variable msg_notify;
        std::condition_variable msg_space;
        std::jthread msg_thread;
        size_t msg_head{}, msg_tail{}, msg_count{};
        auto msg_loop() -> void;
        auto msg_dequeue() -> std::optional<MsgEvent>;
        auto msg_dispatch(UINT msg, WPARAM wparam, LPARAM lparam) -> void;

        IVec2 resize_size{};
        std::atomic_bool dirty{true};
        std::atomic_bool pending{};
        std::atomic_bool resize_pending{false};
        auto mark_dirty() -> void;

        std::atomic<std::weak_ptr<widget::Widget>> focused{};
        std::atomic<std::shared_ptr<widget::Widget>> root{};
    };
} // namespace neko::engine
