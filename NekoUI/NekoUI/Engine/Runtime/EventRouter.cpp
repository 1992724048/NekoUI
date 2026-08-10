#include "EventRouter.hpp"

#include "../Core/Context.hpp"
#include "../Tree/HitTester.hpp"
#include "InvalidationTracker.hpp"
#include "RenderScheduler.hpp"
#include "../Tree/TreeManager.hpp"

#include <cstdio>
#include <utility>

#include "../../Backend/DirectX11.hpp"
#include "../../Style/ColorScheme.hpp"
#include "../../Widget/Widget.hpp"

namespace neko::engine {
    EventRouter::EventRouter(std::weak_ptr<TreeManager> tree,
                             std::weak_ptr<HitTester> hit_tester,
                             std::weak_ptr<device::Mouse> mouse,
                             std::weak_ptr<device::Keyboard> keyboard,
                             std::weak_ptr<Context> context,
                             std::weak_ptr<backend::DirectX11> backend,
                             const std::shared_ptr<RenderScheduler>& scheduler,
                             std::function<void()> destroy_handler,
                             std::weak_ptr<InvalidationTracker> invalidation) :
        tree_(std::move(tree)),
        hit_tester_(std::move(hit_tester)),
        mouse_(std::move(mouse)),
        keyboard_(std::move(keyboard)),
        context_(std::move(context)),
        backend_(std::move(backend)),
        scheduler_(scheduler),
        destroy_handler_(std::move(destroy_handler)),
        invalidation_(std::move(invalidation)) {}

    auto EventRouter::dispatch(const platform::Event& event) const -> void {
        std::visit(platform::Overloaded{
                       [&](const device::MouseMoveEvent& e) -> void {
                           if (const auto mouse = mouse_.lock()) {
                               mouse->handle(e);
                           }
                           handle_input(event);
                       },
                       [&](const device::MouseButtonEvent& e) -> void {
                           if (const auto mouse = mouse_.lock()) {
                               mouse->handle(e);
                           }
                           handle_input(event);
                       },
                       [&](const device::MouseWheelEvent& e) -> void {
                           if (const auto mouse = mouse_.lock()) {
                               mouse->handle(e);
                           }
                           handle_input(event);
                       },
                       [&](const device::KeyEvent& e) -> void {
                           if (const auto keyboard = keyboard_.lock()) {
                               keyboard->handle(e);
                           }
                           handle_input(event);
                       },
                       [&](const device::CharEvent& e) -> void {
                           if (const auto keyboard = keyboard_.lock()) {
                               keyboard->handle(e);
                           }
                           handle_input(event);
                       },
                       [&](const platform::ResizeEvent& e) -> void {
                           handle_resize(e);
                       },
                       [&](const platform::DpiChangeEvent& e) -> void {
                           handle_dpi_change(e);
                       },
                       [&](const platform::ThemeChangedEvent& e) -> void {
                           handle_theme_change(e);
                       },
                       [&](const platform::DestroyEvent&) -> void {
                           handle_destroy();
                       },
                   },
                   event);

        const auto invalidation = invalidation_.lock();
        if (invalidation && invalidation->is_dirty() && !scheduler_.expired()) {
            scheduler_.lock()->request_frame();
        }
    }

    auto EventRouter::handle_input(const platform::Event& event) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        const auto hit_tester = hit_tester_.lock();
        if (!context || !mouse || !hit_tester) {
            return;
        }
        if (std::holds_alternative<device::MouseMoveEvent>(event)) {
            const auto& move = std::get<device::MouseMoveEvent>(event);
            const auto target = hit_tester->hit_test(*mouse);
            const auto prev = last_mouse_target_.lock();
            const bool repatch = prev && (!target || prev.get() != target->get());
            std::printf("[Router] pos=(%d,%d) target=%s prev=%s repatch=%d\n", move.x, move.y, target ? target->get()->path().c_str() : "null", prev ? prev->path().c_str() : "null", repatch);
            if (repatch) {
                prev->input(*context, event);
            }
            last_mouse_target_ = target ? std::weak_ptr<widget::Widget>{*target} : std::weak_ptr<widget::Widget>{};
            if (target) {
                (*target)->input(*context, event);
            }
            return;
        }
        if (const auto target = hit_tester->hit_test(*mouse)) {
            (*target)->input(*context, event);
        }
    }

    auto EventRouter::handle_resize(const platform::ResizeEvent& e) const -> void {
        if (!scheduler_.expired()) {
            scheduler_.lock()->set_pending_size(e.width, e.height);
        }
        if (const auto invalidation = invalidation_.lock()) {
            invalidation->mark_dirty();
        }
    }

    auto EventRouter::handle_dpi_change(const platform::DpiChangeEvent& e) const -> void {
        if (const auto backend = backend_.lock()) {
            backend->set_dpi(e.dpi);
        }
        if (const auto mouse = mouse_.lock()) {
            mouse->set_dpi(e.dpi);
        }
        if (const auto invalidation = invalidation_.lock()) {
            invalidation->mark_dirty();
        }
    }

    auto EventRouter::handle_theme_change(const platform::ThemeChangedEvent& e) const -> void {
        if (const auto context = context_.lock()) {
            context->scheme = (e.mode == platform::ThemeMode::Dark) ? style::ColorScheme::dark(e.color) : style::ColorScheme::light(e.color);
        }
        if (const auto invalidation = invalidation_.lock()) {
            invalidation->mark_dirty();
        }
    }

    auto EventRouter::handle_destroy() const -> void {
        if (destroy_handler_) {
            destroy_handler_();
        }
    }
}
