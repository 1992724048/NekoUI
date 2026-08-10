// 2026-08-10 09:20:42

#include "EventRouter.hpp"

#include "../Core/Context.hpp"
#include "../Tree/HitTester.hpp"
#include "InvalidationTracker.hpp"
#include "RenderScheduler.hpp"
#include "../Tree/TreeManager.hpp"

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
                           handle_mouse_move(e);
                       },
                       [&](const device::MouseButtonEvent& e) -> void {
                           if (const auto mouse = mouse_.lock()) {
                               mouse->handle(e);
                           }
                           handle_mouse_button(e);
                       },
                       [&](const device::MouseWheelEvent& e) -> void {
                           if (const auto mouse = mouse_.lock()) {
                               mouse->handle(e);
                           }
                           handle_mouse_wheel(e);
                       },
                       [&](const device::KeyEvent& e) -> void {
                           if (const auto keyboard = keyboard_.lock()) {
                               keyboard->handle(e);
                           }
                           handle_key(e);
                       },
                       [&](const device::CharEvent& e) -> void {
                           if (const auto keyboard = keyboard_.lock()) {
                               keyboard->handle(e);
                           }
                           handle_char(e);
                       },
                       [&](const platform::ImeCompositionEvent& e) -> void {
                           handle_ime(e);
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

    auto EventRouter::handle_key(const device::KeyEvent& e) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        if (!context || !mouse) {
            return;
        }
        if (const auto focus = focused_target()) {
            if (!try_tab_navigate(context, e)) {
                focus->input(*context, e);
            }
            return;
        }
        if (const auto target = hit_target(*mouse)) {
            (*target)->input(*context, e);
        }
    }

    auto EventRouter::handle_char(const device::CharEvent& e) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        if (!context || !mouse) {
            return;
        }
        if (const auto focus = focused_target()) {
            focus->input(*context, e);
            return;
        }
        if (const auto target = hit_target(*mouse)) {
            (*target)->input(*context, e);
        }
    }

    auto EventRouter::handle_ime(const platform::ImeCompositionEvent& e) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        if (!context || !mouse) {
            return;
        }
        if (const auto focus = focused_target()) {
            focus->input(*context, e);
            return;
        }
        if (const auto target = hit_target(*mouse)) {
            (*target)->input(*context, e);
        }
    }

    auto EventRouter::handle_mouse_move(const device::MouseMoveEvent& e) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        if (!context || !mouse) {
            return;
        }
        const auto target = hit_target(*mouse);
        const auto prev = last_mouse_target_.lock();
        if (prev && (!target || prev.get() != target->get())) {
            prev->set_hovered(false);
        }
        last_mouse_target_ = target ? std::weak_ptr{*target} : std::weak_ptr<widget::Widget>{};
        if (target) {
            (*target)->set_hovered(true);
            (*target)->input(*context, e);
        }
    }

    auto EventRouter::handle_mouse_button(const device::MouseButtonEvent& e) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        if (!context || !mouse) {
            return;
        }
        if (const auto target = hit_target(*mouse)) {
            if (const auto tree = tree_.lock()) {
                tree->set_focus(*target);
            }
            (*target)->input(*context, e);
        }
    }

    auto EventRouter::handle_mouse_wheel(const device::MouseWheelEvent& e) const -> void {
        const auto context = context_.lock();
        const auto mouse = mouse_.lock();
        if (!context || !mouse) {
            return;
        }
        if (const auto target = hit_target(*mouse)) {
            (*target)->input(*context, e);
        }
    }

    auto EventRouter::hit_target(const device::Mouse& mouse) const -> std::optional<std::shared_ptr<widget::Widget>> {
        const auto hit_tester = hit_tester_.lock();
        return hit_tester ? hit_tester->hit_test(mouse) : std::nullopt;
    }

    auto EventRouter::focused_target() const -> std::shared_ptr<widget::Widget> {
        const auto tree = tree_.lock();
        return tree ? tree->get_focus().lock() : nullptr;
    }

    auto EventRouter::try_tab_navigate(const std::shared_ptr<Context>& context, const device::KeyEvent& e) const -> bool {
        if (!(e.pressed && e.key == 0x09)) { // VK_TAB
            return false;
        }
        const auto tree = tree_.lock();
        const auto next = tree ? tree->next_focus() : std::weak_ptr<widget::Widget>{};
        if (!next.lock()) {
            return false;
        }
        tree->set_focus(next);
        if (context->mark_dirty) {
            context->mark_dirty();
        }
        return true;
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
            context->scheme = e.mode == platform::ThemeMode::Dark ? style::ColorScheme::dark(e.color) : style::ColorScheme::light(e.color);
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
