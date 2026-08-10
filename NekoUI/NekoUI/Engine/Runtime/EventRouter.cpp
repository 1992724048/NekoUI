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
                       [&](const platform::ImeCompositionEvent&) -> void {
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
        // 键盘/字符/IME 事件优先派发给焦点控件（Tab = 焦点导航）
        if (std::holds_alternative<device::KeyEvent>(event) || std::holds_alternative<device::CharEvent>(event) || std::holds_alternative<platform::ImeCompositionEvent>(event)) {
            const auto tree = tree_.lock();
            const auto focus = tree ? tree->get_focus().lock() : nullptr;
            if (focus) {
                if (const auto* key = std::get_if<device::KeyEvent>(&event); key != nullptr && key->pressed && key->key == 0x09) {  // VK_TAB
                    const auto next = tree->next_focus();
                    if (next.lock()) {
                        tree->set_focus(next);  // 实际切换焦点（focused_ 更新）
                        if (context->mark_dirty) {
                            context->mark_dirty();
                        }
                    }
                    return;
                }
                focus->input(*context, event);
                return;
            }
        }
        if (std::holds_alternative<device::MouseMoveEvent>(event)) {
            const auto target = hit_tester->hit_test(*mouse);
            const auto prev = last_mouse_target_.lock();
            if (prev && (!target || prev.get() != target->get())) {
                prev->set_hovered(false);
            }
            last_mouse_target_ = target ? std::weak_ptr{*target} : std::weak_ptr<widget::Widget>{};
            if (target) {
                (*target)->set_hovered(true);
                (*target)->input(*context, event);
            }
            return;
        }
        if (const auto target = hit_tester->hit_test(*mouse)) {
            if (std::holds_alternative<device::MouseButtonEvent>(event)) {
                if (const auto tree = tree_.lock()) {
                    tree->set_focus(*target);
                }
            }
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
