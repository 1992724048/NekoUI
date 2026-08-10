// 2026-08-10 10:17:56

#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <type_traits>

#include "Context.hpp"
#include "../Runtime/EventRouter.hpp"
#include "../Tree/HitTester.hpp"
#include "../Runtime/InvalidationTracker.hpp"
#include "../Runtime/MsgPump.hpp"
#include "../Runtime/RenderScheduler.hpp"
#include "../Tree/TreeManager.hpp"
#include "../Tree/WidgetBuilder.hpp"

#include "../../Type.hpp"

#include "../../Widget/Widget.hpp"

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

        template<std::derived_from<widget::Widget> T, typename BuilderFn>
        auto set_root_widget(BuilderFn&& builder) -> std::shared_ptr<T> {
            auto stored_builder = std::forward<BuilderFn>(builder);
            root_builder_ = [builder = stored_builder](widget::Widget& root) -> void {
                builder(root);
            };
            rebuild_root_ = [this, builder = stored_builder]() -> void {
                const auto root = std::make_shared<T>(*context);
                tree_manager_->set_root(*context, root);
                context->root = root;
                builder(*root);
            };

            const auto root = std::make_shared<T>(*context);
            tree_manager_->set_root(*context, root);
            widget_builder_->build(*context);
            context->root = root;
            root_builder_(*root);
            render_scheduler_->request_frame();
            return root;
        }

        auto clear() -> void;
        auto get_msg_pump() -> std::weak_ptr<MsgPump>;
        auto get_render_scheduler() -> std::weak_ptr<RenderScheduler>;
        auto rebuild() -> void;
        auto schedule_rebuild() -> void;
        [[nodiscard]] auto get_native_handle() const -> Handle;
        [[nodiscard]] auto get_context() const -> Context&;
    private:
        std::shared_ptr<Context> context;
        std::shared_ptr<backend::DirectX11> backend;
        Handle native_handle_{};
        std::shared_ptr<device::Mouse> mouse;
        std::shared_ptr<device::Keyboard> keyboard;

        auto render_frame() -> void;
        static auto draw_widget(widget::Widget& w, Context& context, backend::DirectX11& backend) -> void;

        std::shared_ptr<InvalidationTracker> invalidation_;
        std::shared_ptr<TreeManager> tree_manager_;
        std::shared_ptr<WidgetBuilder> widget_builder_;
        std::shared_ptr<HitTester> hit_tester_;
        std::shared_ptr<RenderScheduler> render_scheduler_;
        std::shared_ptr<MsgPump> msg_pump_;
        std::shared_ptr<EventRouter> event_router_;
        std::atomic_bool tree_dirty_{false};

        std::function<void(widget::Widget&)> root_builder_;
        std::function<void()> rebuild_root_;
    };
}
