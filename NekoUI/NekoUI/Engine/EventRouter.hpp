#pragma once

#include <Windows.h>
#include <memory>

namespace neko::engine {
    class WidgetTree;
    class RenderScheduler;
    class InvalidationTracker;
    struct Context;
} // namespace neko::engine

namespace neko::device {
    struct Mouse;
}

namespace neko::device {
    struct Keyboard;
}

namespace neko::backend {
    class Backend;
}

namespace neko::engine {
    class EventRouter {
    public:
        EventRouter(WidgetTree& tree, device::Mouse& mouse, device::Keyboard& keyboard, Context& context, backend::Backend& backend, const std::shared_ptr<RenderScheduler>& scheduler, InvalidationTracker& invalidation);

        auto dispatch(UINT msg, WPARAM wparam, LPARAM lparam) const -> void;
    private:
        auto handle_mouse(UINT msg, WPARAM wparam, LPARAM lparam) const -> void;
        auto handle_keyboard(UINT msg, WPARAM wparam, LPARAM lparam) const -> void;
        auto handle_dpi_change(WPARAM wparam) const -> void;
        auto handle_resize(LPARAM lparam) const -> void;

        WidgetTree& tree_;
        device::Mouse& mouse_;
        device::Keyboard& keyboard_;
        Context& context_;
        backend::Backend& backend_;
        std::weak_ptr<RenderScheduler> scheduler_;
        InvalidationTracker& invalidation_;
    };
} // namespace neko::engine
