#include "EventRouter.hpp"

#include "Context.hpp"
#include "InvalidationTracker.hpp"
#include "RenderScheduler.hpp"
#include "WidgetTree.hpp"

#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    EventRouter::EventRouter(WidgetTree& tree,
                             device::Mouse& mouse,
                             device::Keyboard& keyboard,
                             Context& context,
                             backend::Backend& backend,
                             const std::shared_ptr<RenderScheduler>& scheduler,
                             InvalidationTracker& invalidation) :
        tree_(tree),
        mouse_(mouse),
        keyboard_(keyboard),
        context_(context),
        backend_(backend),
        scheduler_(scheduler),
        invalidation_(invalidation) {}

    auto EventRouter::dispatch(const UINT msg, const WPARAM wparam, const LPARAM lparam) const -> void {
        mouse_.handle(msg, wparam, lparam);
        keyboard_.handle(msg, wparam, lparam);

        switch (msg) {
            case WM_SIZE:
                handle_resize(lparam);
                break;
            case WM_DPICHANGED:
                handle_dpi_change(wparam);
                break;
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            case WM_MOUSEWHEEL:
                handle_mouse(msg, wparam, lparam);
                break;
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                handle_keyboard(msg, wparam, lparam);
                break;
            case WM_TIMER: default:
                break;
        }

        if (invalidation_.is_dirty() && !scheduler_.expired()) {
            scheduler_.lock()->request_frame();
        }
    }

    auto EventRouter::handle_mouse(const UINT msg, const WPARAM wparam, const LPARAM lparam) const -> void {
        const auto root = tree_.get_root();
        if (!root) {
            return;
        }
        if (!root->raw_event(context_, msg, wparam, lparam)) {
            static_cast<void>(root->hit_test(mouse_));
        }
    }

    auto EventRouter::handle_keyboard(const UINT msg, const WPARAM wparam, const LPARAM lparam) const -> void {
        if (wparam == VK_TAB) {
            return;
        }
        const auto focused = tree_.get_focus().lock();
        if (focused) {
            focused->raw_event(context_, msg, wparam, lparam);
        }
    }

    auto EventRouter::handle_dpi_change(const WPARAM wparam) const -> void {
        const UINT dpi = LOWORD(wparam);
        backend_.set_dpi(dpi);
        mouse_.set_dpi(dpi);
        invalidation_.mark_dirty();
    }

    auto EventRouter::handle_resize(const LPARAM lparam) const -> void {
        if (!scheduler_.expired()) {
            scheduler_.lock()->set_pending_size(LOWORD(lparam), HIWORD(lparam));
        }
        invalidation_.mark_dirty();
    }
} // namespace neko::engine
