#pragma once

#include <functional>
#include <memory>

#include "../../Platform/Event.hpp"

namespace neko::engine {
    class TreeManager;
    class HitTester;
    class RenderScheduler;
    class InvalidationTracker;
    struct Context;
}

namespace neko::device {
    struct Mouse;
    struct Keyboard;
}

namespace neko::backend {
    class DirectX11;
}

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    class EventRouter {
    public:
        EventRouter(std::weak_ptr<TreeManager> tree,
                    std::weak_ptr<HitTester> hit_tester,
                    std::weak_ptr<device::Mouse> mouse,
                    std::weak_ptr<device::Keyboard> keyboard,
                    std::weak_ptr<Context> context,
                    std::weak_ptr<backend::DirectX11> backend,
                    const std::shared_ptr<RenderScheduler>& scheduler,
                    std::function<void()> destroy_handler,
                    std::weak_ptr<InvalidationTracker> invalidation);

        auto dispatch(const platform::Event& event) const -> void;
    private:
        auto handle_input(const platform::Event& event) const -> void;
        auto handle_resize(const platform::ResizeEvent& e) const -> void;
        auto handle_dpi_change(const platform::DpiChangeEvent& e) const -> void;
        auto handle_theme_change(const platform::ThemeChangedEvent& e) const -> void;
        auto handle_destroy() const -> void;

        std::weak_ptr<TreeManager> tree_;
        std::weak_ptr<HitTester> hit_tester_;
        std::weak_ptr<device::Mouse> mouse_;
        std::weak_ptr<device::Keyboard> keyboard_;
        std::weak_ptr<Context> context_;
        std::weak_ptr<backend::DirectX11> backend_;
        std::weak_ptr<RenderScheduler> scheduler_;
        std::function<void()> destroy_handler_;
        std::weak_ptr<InvalidationTracker> invalidation_;
        mutable std::weak_ptr<widget::Widget> last_mouse_target_;
    };
}
