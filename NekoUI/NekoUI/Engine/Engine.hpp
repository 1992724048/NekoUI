#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <thread>
#include <tuple>
#include <type_traits>

#include "Context.hpp"
#include "MsgLoop.hpp"

#include "../Type.hpp"

#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

#include "Component/Animation.hpp"
#include "Component/ValueState.hpp"

namespace neko::engine {
    using namespace neko::type;

    class Engine final : public MsgLoop {
    public:
        explicit Engine(HWND hwnd);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;
        Engine(Engine&&) = delete;
        auto operator=(Engine&&) -> Engine& = delete;

        template<typename T, typename... Args> requires std::is_base_of_v<widget::Widget, T>
        auto set_root_widget(Args&&... args) -> std::shared_ptr<T> {
            const std::shared_ptr<T> widget = std::make_shared<T>(*context, std::forward<Args>(args)...);
            root = widget;
            context->root = widget;
            frame();
            return widget;
        }

        // 清空引擎资源
        auto clear() -> void;

        // 刷新一帧
        auto frame() -> void;

        // 投递窗口消息
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void override;
    private:
        auto del_widget(const std::weak_ptr<widget::Widget>& widget) -> bool;
        auto reg_widget(const std::weak_ptr<widget::Widget>& widget) -> bool;

        auto bind_animation(animation::AnimationBase& anim) -> void;
        auto bind_value_state(state::ValueStateBase& state) -> void;

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

        auto msg_loop() -> void override;
        auto msg_dequeue() -> std::optional<MsgEvent> override;
        auto msg_dispatch(UINT msg, WPARAM wparam, LPARAM lparam) -> void override;

        Vec2I resize_size{};
        std::atomic_bool dirty{true};
        std::atomic_bool pending{};
        std::atomic_bool resize_pending{false};
        auto mark_dirty() -> void;

        std::shared_mutex id_map_mutex;
        std::map<std::string, std::weak_ptr<widget::Widget>> id_widgets{};

        std::shared_mutex dirty_list_mutex;
        std::vector<std::weak_ptr<widget::Widget>> dirty_list;
        auto mark_widget_dirty(const std::weak_ptr<widget::Widget>& widget) -> void;

        std::atomic<std::weak_ptr<widget::Widget>> focused{};
        std::atomic<std::shared_ptr<widget::Widget>> root{};
    };
}
