#pragma once

#include <Windows.h>

namespace neko::engine {
    class WidgetTree;
    class RenderScheduler;
    class InvalidationTracker;
    struct Context;
} // namespace neko::engine

namespace neko::mouse {
    struct Mouse;
}

namespace neko::keyboard {
    struct Keyboard;
}

namespace neko::backend {
    class Backend;
}

namespace neko::engine {
    class EventRouter {
    public:
        EventRouter(WidgetTree& tree, mouse::Mouse& mouse, keyboard::Keyboard& keyboard, Context& context, backend::Backend& backend, RenderScheduler& scheduler, InvalidationTracker& invalidation);

        auto dispatch(UINT msg, WPARAM wparam, LPARAM lparam) const -> void;
    private:
        auto handle_mouse(UINT msg, WPARAM wparam, LPARAM lparam) const -> void;
        auto handle_keyboard(UINT msg, WPARAM wparam, LPARAM lparam) const -> void;
        auto handle_dpi_change(WPARAM wparam) const -> void;
        auto handle_resize(LPARAM lparam) const -> void;

        WidgetTree& tree_;
        mouse::Mouse& mouse_;
        keyboard::Keyboard& keyboard_;
        Context& context_;
        backend::Backend& backend_;
        RenderScheduler& scheduler_;
        InvalidationTracker& invalidation_;
    };
} // namespace neko::engine
