#pragma once

#include <functional>
#include <memory>

#include "../Platform/Platform.hpp"

namespace neko::engine {
    class WidgetTree;
    class RenderScheduler;
    class InvalidationTracker;
    struct Context;
}

namespace neko::device {
    struct Mouse;
    struct Keyboard;
}

namespace neko::backend {
    class Backend;
}

namespace neko::engine {
    class EventRouter {
    public:
        EventRouter(WidgetTree& tree, device::Mouse& mouse, device::Keyboard& keyboard, Context& context, backend::Backend& backend, const std::shared_ptr<RenderScheduler>& scheduler, std::function<void()> destroy_handler, InvalidationTracker& invalidation);

        auto dispatch(const platform::Event& event) const -> void;
    private:
        auto handle_input(const platform::Event& event) const -> void;
        auto handle_resize(const platform::ResizeEvent& e) const -> void;
        auto handle_dpi_change(const platform::DpiChangeEvent& e) const -> void;
        auto handle_theme_change(const platform::ThemeChangedEvent& e) const -> void;
        auto handle_destroy() const -> void;

        WidgetTree& tree_;
        device::Mouse& mouse_;
        device::Keyboard& keyboard_;
        Context& context_;
        backend::Backend& backend_;
        std::weak_ptr<RenderScheduler> scheduler_;
        std::function<void()> destroy_handler_;
        InvalidationTracker& invalidation_;
    };
}
