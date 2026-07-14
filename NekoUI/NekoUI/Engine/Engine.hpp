#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>

#include "Context.hpp"

#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

#include "Component/ColorSeed.hpp"

namespace neko::engine {
    class Engine final {
    public:
        explicit Engine(HWND hwnd);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;

        template<typename T, typename... Args> requires std::is_base_of_v<widget::Widget, T>
        auto set_root_widget(Args&&... args) -> std::weak_ptr<T> {
            const std::shared_ptr<widget::Widget> widget = std::make_shared<T>(this, std::forward<Args>(args)...);
            root = widget;
            frame();
            return std::static_pointer_cast<T>(widget);
        }

        template<typename T, typename... Args> requires std::is_base_of_v<widget::Widget, T>
        auto create_widget(Args&&... args) -> std::weak_ptr<T> {
            const std::shared_ptr<widget::Widget> widget = std::make_shared<T>(this, std::forward<Args>(args)...);
            root = widget;
            frame();
            return std::static_pointer_cast<T>(widget);
        }

        auto clear() -> void;
        auto frame() -> void;
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;
    protected:
        friend widget::Widget;

        auto del(widget::Widget* widget) -> bool;
    private:
        static constexpr size_t MSG_QUEUE_MAX = 32;

        using MsgEvent = std::tuple<UINT, WPARAM, LPARAM>;
        using Clock = std::chrono::steady_clock;

        auto render_loop() -> void;
        auto render_wait() -> void;
        auto render_frame() -> void;

        auto msg_loop() -> void;
        auto msg_dequeue() -> std::optional<MsgEvent>;
        auto msg_dispatch(UINT msg, WPARAM wparam, LPARAM lparam) -> void;

        auto anim_inc() -> void;
        auto anim_dec() -> void;

        auto mark_dirty() -> void;

        std::unique_ptr<Context> context{};
        std::unique_ptr<backend::Backend> backend{};
        std::shared_ptr<color::ColorScheme> scheme{};
        std::shared_ptr<mouse::Mouse> mouse;
        std::shared_ptr<keyboard::Keyboard> keyboard;

        std::mutex render_lock;
        std::condition_variable render_notify;
        std::jthread render_thread;

        std::array<MsgEvent, MSG_QUEUE_MAX> msg_queue{};
        std::mutex msg_mutex;
        std::condition_variable msg_notify;
        std::condition_variable msg_space;
        std::jthread msg_thread;
        size_t msg_head{}, msg_tail{}, msg_count{};

        glm::ivec2 resize_size{};
        std::atomic_int animation{};
        std::atomic_bool dirty{true};
        std::atomic_bool pending{};
        std::atomic_bool resize_pending{false};

        std::atomic<std::weak_ptr<widget::Widget>> focused{};
        std::atomic<std::shared_ptr<widget::Widget>> root{};
    };
} // namespace neko::engine
