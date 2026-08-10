// 2026-08-02 04:25:14

#include "Engine.hpp"
#include "EventRouter.hpp"
#include "InvalidationTracker.hpp"
#include "MsgPump.hpp"
#include "RenderScheduler.hpp"
#include "TreeManager.hpp"
#include "WidgetBuilder.hpp"
#include "WidgetVisitor.hpp"

#include <Windows.h>
#include <cmath>
#include <optional>

#include "../Backend/DirectX11.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    Engine::Engine(std::unique_ptr<backend::DirectX11> backend) :
        backend{std::move(backend)},
        native_handle_(this->backend->get_native_handle()) {
        context = std::make_shared<Context>();
        mouse = std::make_shared<device::Mouse>();
        keyboard = std::make_shared<device::Keyboard>();
        invalidation_ = std::make_shared<InvalidationTracker>();
        tree_manager_ = std::make_shared<TreeManager>();
        widget_builder_ = std::make_shared<WidgetBuilder>(tree_manager_);
        hit_tester_ = std::make_shared<HitTester>(tree_manager_);
        context->tree_manager = tree_manager_;

        const auto initial_dpi = static_cast<unsigned int>(std::round(this->backend->get_dpi_scale() * 96.0F));
        mouse->set_dpi(initial_dpi);

        context->anim_inc = [this]() -> void {
            invalidation_->anim_inc();
            if (render_scheduler_) {
                render_scheduler_->request_frame();
            }
        };
        context->anim_dec = [this]() -> void {
            invalidation_->anim_dec();
        };

        context->widget_tree_changed = [this]() -> void {
            schedule_rebuild();
        };

        context->mark_dirty = [this]() -> void {
            invalidation_->mark_dirty();
        };
        context->widget_dirty = [this](const std::weak_ptr<widget::Widget>& widget) -> void {
            invalidation_->mark_widget_dirty(widget);
        };

        context->mouse = mouse;
        context->keyboard = keyboard;
        context->native_handle = native_handle_;

        render_scheduler_ = std::make_shared<RenderScheduler>([this]() -> void {
                                                                  render_frame();
                                                              },
                                                              invalidation_);
        event_router_ = std::make_shared<EventRouter>(tree_manager_,
                                                      hit_tester_,
                                                      mouse,
                                                      keyboard,
                                                      context,
                                                      this->backend,
                                                      render_scheduler_,
                                                      [this]() -> void {
                                                          clear();
                                                      },
                                                      invalidation_);
        msg_pump_ = std::make_shared<MsgPump>([router = std::weak_ptr{event_router_}](const platform::Event& event) -> void {
            if (const auto locked = router.lock()) {
                locked->dispatch(event);
            }
        });
    }

    Engine::~Engine() {
        clear();
        event_router_.reset();
    }

    auto Engine::clear() -> void {
        if (msg_pump_) {
            msg_pump_->stop();
        }
        if (render_scheduler_) {
            render_scheduler_->stop();
            render_scheduler_.reset();
        }
        tree_manager_->clear();
        invalidation_->clear();
    }

    auto Engine::get_msg_pump() -> std::weak_ptr<MsgPump> {
        return msg_pump_;
    }

    auto Engine::get_render_scheduler() -> std::weak_ptr<RenderScheduler> {
        return render_scheduler_;
    }

    auto Engine::rebuild() const -> void {
        widget_builder_->build(*context);
        if (render_scheduler_) {
            render_scheduler_->request_frame();
        }
    }

    auto Engine::schedule_rebuild() -> void {
        tree_dirty_.store(true, std::memory_order_relaxed);
        if (render_scheduler_) {
            render_scheduler_->request_frame();
        }
    }

    auto Engine::get_native_handle() const -> Handle {
        return native_handle_;
    }

    auto Engine::get_context() const -> Context& {
        return *context;
    }

    auto Engine::render_frame() -> void {
        if (const auto resize = render_scheduler_->consume_resize()) {
            backend->resize(*resize);
        }

        if (tree_dirty_.exchange(false, std::memory_order_acq_rel)) {
            widget_builder_->build(*context);
        }

        auto size = render_scheduler_->pending_size();
        if (size.width <= 0 || size.height <= 0) {
            RECT client{};
            if (GetClientRect(static_cast<HWND>(native_handle_), &client) != 0) {
                size = {.x = client.right - client.left, .y = client.bottom - client.top};
            }
        }

        const auto root = tree_manager_->get_root();
        if (!root) {
            invalidation_->clear();
            return;
        }

        std::shared_lock lock(tree_manager_->mutex_);

        root->layout({.x = 0, .y = 0, .z = size.width, .w = size.height}, *context);

        backend->begin();
        draw_widget(*root, *context, *backend);
        backend->end();

        invalidation_->clear();
    }

    auto Engine::draw_widget(widget::Widget& w, Context& context, backend::DirectX11& backend) -> void {
        w.draw(w.get_bounds(), context, backend);
        visit_children(w,
                       [&](const std::shared_ptr<widget::Widget>& child) -> void {
                           draw_widget(*child, context, backend);
                       });
    }
}
