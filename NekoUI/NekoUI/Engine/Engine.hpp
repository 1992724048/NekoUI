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
            tree_manager_->set_root(*context, widget);
            widget_builder_->build(*context);
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
        // 生命周期契约：Engine 拥有全部子系统（shared_ptr），观察者
        // （EventRouter/HitTester/WidgetBuilder/RenderScheduler/MsgPump/Context/Widget）
        // 持 weak_ptr；成员声明顺序仍保证观察者先于被观察者析构，
        // weak_ptr 过期只是最后的运行时安全网（lock 失败必须安全降级，不得 UB）。
        // 新增成员时保持此顺序。
        std::shared_ptr<Context> context{};
        std::shared_ptr<backend::DirectX11> backend{};
        Handle native_handle_{};
        std::shared_ptr<device::Mouse> mouse;
        std::shared_ptr<device::Keyboard> keyboard;

        auto render_frame() -> void;
        static auto draw_widget(widget::Widget& w, Context& context, backend::DirectX11& backend) -> void;

        std::shared_ptr<InvalidationTracker> invalidation_{};
        std::shared_ptr<TreeManager> tree_manager_{};
        std::shared_ptr<WidgetBuilder> widget_builder_{};
        std::shared_ptr<HitTester> hit_tester_{};
        std::shared_ptr<RenderScheduler> render_scheduler_{};
        std::shared_ptr<MsgPump> msg_pump_{};
        std::shared_ptr<EventRouter> event_router_{};
        std::atomic_bool tree_dirty_{false};
    };
}
