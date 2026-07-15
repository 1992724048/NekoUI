#include "Engine.hpp"
#include "EventRouter.hpp"
#include "InvalidationTracker.hpp"
#include "MsgPump.hpp"
#include "RenderScheduler.hpp"
#include "WidgetTree.hpp"

#include <cmath>
#include <minwindef.h>
#include <mutex>
#include <optional>
#include <thread>
#include <windef.h>

#include "../Widget/Widget.hpp"

namespace neko::engine {
    Engine::Engine(const HWND hwnd) {
        backend = std::make_unique<backend::Backend>(hwnd);
        context = std::make_unique<Context>();

        mouse = std::make_shared<mouse::Mouse>();
        keyboard = std::make_shared<keyboard::Keyboard>();

        const UINT initial_dpi = static_cast<UINT>(std::round(backend->get_dpi_scale() * 96.0F));
        mouse->set_dpi(initial_dpi);

        context->anim_inc = std::bind(InvalidationTracker::anim_inc, &invalidation_);
        context->anim_dec = std::bind(InvalidationTracker::anim_dec, &invalidation_);

        context->reg_widget = [this](const std::weak_ptr<widget::Widget>& w) {
            widget_tree_.register_widget(w);
            frame();
        };
        context->del_widget = [this](const std::weak_ptr<widget::Widget>& w) {
            widget_tree_.unregister_widget(w);
            frame();
        };

        context->mark_dirty = [this] {
            invalidation_.mark_dirty();
        };
        context->widget_dirty = [this](const std::weak_ptr<widget::Widget>& w) {
            invalidation_.mark_widget_dirty(w);
        };

        context->mouse = mouse;
        context->keyboard = keyboard;

        render_scheduler_ = std::make_unique<RenderScheduler>([this] {
                                                                  render_frame();
                                                              },
                                                              invalidation_);

        event_router_ = std::make_unique<EventRouter>(widget_tree_, *mouse, *keyboard, *context, *backend, *render_scheduler_, invalidation_);

        msg_pump_ = std::make_unique<MsgPump>([this](const UINT msg, const WPARAM wparam, const LPARAM lparam) {
            event_router_->dispatch(msg, wparam, lparam);
        });
    }

    Engine::~Engine() {
        clear();
    }

    auto Engine::clear() -> void {
        if (msg_pump_) {
            msg_pump_->stop();
        }
        event_router_.reset();
        if (render_scheduler_) {
            render_scheduler_->stop();
            render_scheduler_.reset();
        }
        widget_tree_.clear();
        invalidation_.clear();
    }

    auto Engine::frame() const -> void {
        render_scheduler_->request_frame();
    }

    auto Engine::push_msg(const UINT msg, const WPARAM wparam, const LPARAM lparam) const -> void {
        msg_pump_->push_msg(msg, wparam, lparam);
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
} // namespace neko::engine
