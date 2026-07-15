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
#include "EventRouter.hpp"
#include "InvalidationTracker.hpp"
#include "MsgPump.hpp"
#include "RenderScheduler.hpp"
#include "WidgetTree.hpp"

#include "../Type.hpp"

#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

#include "Component/Animation.hpp"
#include "Component/ValueState.hpp"

namespace neko::engine {
    using namespace neko::type;

    class Engine final {
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
            widget_tree_.set_root(widget);
            context->root = widget;
            frame();
            return widget;
        }

        // 清空引擎资源
        auto clear() -> void;

        // 刷新一帧
        auto frame() -> void;

        // 投递窗口消息
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;
    private:
        auto bind_animation(animation::AnimationBase& anim) -> void;
        auto bind_value_state(state::ValueStateBase& state) -> void;

        std::unique_ptr<Context> context{};
        std::unique_ptr<backend::Backend> backend{};
        std::shared_ptr<mouse::Mouse> mouse;
        std::shared_ptr<keyboard::Keyboard> keyboard;

        auto render_frame() -> void;

        InvalidationTracker invalidation_;
        WidgetTree widget_tree_;
        std::unique_ptr<RenderScheduler> render_scheduler_;
        std::unique_ptr<MsgPump> msg_pump_;
        std::unique_ptr<EventRouter> event_router_;
    };
}
