#include "Engine.hpp"
#include "EventRouter.hpp"
#include "InvalidationTracker.hpp"
#include "MsgPump.hpp"
#include "RenderScheduler.hpp"
#include "WidgetTree.hpp"

#include <cmath>
#include <optional>

#include "../Widget/Widget.hpp"

namespace neko::engine {
    Engine::Engine(std::unique_ptr<backend::Backend> backend) :
        backend{std::move(backend)} {
        context = std::make_unique<Context>();

        native_handle_ = this->backend->get_native_handle();

        mouse = std::make_shared<device::Mouse>();
        keyboard = std::make_shared<device::Keyboard>();

        const auto initial_dpi = static_cast<unsigned int>(std::round(this->backend->get_dpi_scale() * 96.0F));
        mouse->set_dpi(initial_dpi);

        context->anim_inc = std::bind(&InvalidationTracker::anim_inc, &invalidation_);
        context->anim_dec = std::bind(&InvalidationTracker::anim_dec, &invalidation_);

        context->reg_widget = [this](const std::weak_ptr<widget::Widget>& w) -> void {
            widget_tree_.register_widget(w);
            render_scheduler_->request_frame();
        };
        context->del_widget = [this](const std::weak_ptr<widget::Widget>& w) -> void {
            widget_tree_.unregister_widget(w);
            render_scheduler_->request_frame();
        };

        context->mark_dirty = std::bind(&InvalidationTracker::mark_dirty, &invalidation_);
        context->widget_dirty = std::bind(&InvalidationTracker::mark_widget_dirty, &invalidation_, std::placeholders::_1);

        context->mouse = mouse;
        context->keyboard = keyboard;

        render_scheduler_ = std::make_shared<RenderScheduler>(std::bind(&Engine::render_frame, this), invalidation_);
        event_router_ = std::make_unique<EventRouter>(widget_tree_, *mouse, *keyboard, *context, *backend, render_scheduler_, std::bind(&Engine::clear, this), invalidation_);
        msg_pump_ = std::make_shared<MsgPump>(std::bind(&EventRouter::dispatch, event_router_.get(), std::placeholders::_1));
        msg_pump_->push_msg(platform::Platform::instance().query_theme());
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
        backend.reset();
        context.reset();
        mouse.reset();
        keyboard.reset();
        widget_tree_.clear();
        invalidation_.clear();
    }

    auto Engine::get_msg_pump() -> std::weak_ptr<MsgPump> {
        return msg_pump_;
    }

    auto Engine::get_render_scheduler() -> std::weak_ptr<RenderScheduler> {
        return render_scheduler_;
    }

    auto Engine::render_frame() -> void {
        if (const auto resize = render_scheduler_->consume_resize()) {
            backend->resize(*resize);
        }

        backend->begin();
        const auto widget = widget_tree_.get_root();
        if (widget) {
            const auto [x, y] = render_scheduler_->pending_size();
            widget->layout({.x = 0, .y = 0, .width = x, .height = y});
            widget->draw(*context, *backend);
        }
        backend->end();
        invalidation_.clear();
    }
}
