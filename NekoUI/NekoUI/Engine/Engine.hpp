// 2026-08-02 04:25:02

#pragma once
#include <atomic>
#include <memory>
#include <type_traits>

#include "Context.hpp"
#include "EventRouter.hpp"
#include "HitTester.hpp"
#include "InvalidationTracker.hpp"
#include "MsgPump.hpp"
#include "RenderScheduler.hpp"
#include "TreeManager.hpp"
#include "WidgetBuilder.hpp"

#include "../Type.hpp"

#include "../Widget/Widget.hpp"

namespace neko::backend {
    class DirectX11;
}

namespace neko::engine {
    using namespace neko::type;

    class Engine final {
    public:
        explicit Engine(std::unique_ptr<backend::DirectX11> backend);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;
        Engine(Engine&&) = delete;
        auto operator=(Engine&&) -> Engine& = delete;

        template<std::derived_from<widget::Widget> T, typename... Args>
        auto set_root_widget(Args&&... args) -> std::shared_ptr<T> {
            const std::shared_ptr<T> widget = std::make_shared<T>(*context, std::forward<Args>(args)...);
            tree_manager_.set_root(*context, widget);
            widget_builder_.build(*context);
            context->root = widget;
            render_scheduler_->request_frame();
            return widget;
        }

        auto clear() -> void;
        auto get_msg_pump() -> std::weak_ptr<MsgPump>;
        auto get_render_scheduler() -> std::weak_ptr<RenderScheduler>;
        auto rebuild() -> void;
        auto schedule_rebuild() -> void;
        [[nodiscard]] auto get_native_handle() const -> Handle;
        [[nodiscard]] auto get_context() const -> Context&;
    private:
        std::unique_ptr<Context> context{};
        std::unique_ptr<backend::DirectX11> backend{};
        Handle native_handle_{};
        std::shared_ptr<device::Mouse> mouse;
        std::shared_ptr<device::Keyboard> keyboard;

        auto render_frame() -> void;
        static auto draw_widget(widget::Widget& w, Context& context, backend::DirectX11& backend) -> void;

        InvalidationTracker invalidation_;
        TreeManager tree_manager_;
        WidgetBuilder widget_builder_{tree_manager_};
        HitTester hit_tester_{tree_manager_};
        std::shared_ptr<RenderScheduler> render_scheduler_{};
        std::shared_ptr<MsgPump> msg_pump_{};
        std::unique_ptr<EventRouter> event_router_{};
        std::atomic_bool tree_dirty_{false};
    };
}
