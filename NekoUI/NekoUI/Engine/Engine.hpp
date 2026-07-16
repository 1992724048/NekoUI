#pragma once
#include <memory>
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

namespace neko::engine {
    using namespace neko::type;

    class Engine final {
    public:
        explicit Engine(std::unique_ptr<backend::Backend> backend);
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
            render_scheduler_->request_frame();
            return widget;
        }

        auto clear() -> void;
        auto get_msg_pump() -> std::weak_ptr<MsgPump>;
        auto get_render_scheduler() -> std::weak_ptr<RenderScheduler>;
    private:
        std::unique_ptr<Context> context{};
        std::unique_ptr<backend::Backend> backend{};
        std::shared_ptr<device::Mouse> mouse;
        std::shared_ptr<device::Keyboard> keyboard;

        auto render_frame() -> void;

        InvalidationTracker invalidation_;
        WidgetTree widget_tree_;
        std::shared_ptr<RenderScheduler> render_scheduler_{};
        std::shared_ptr<MsgPump> msg_pump_{};
        std::unique_ptr<EventRouter> event_router_{};
    };
}
