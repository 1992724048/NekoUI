#pragma once

#include <functional>
#include <memory>
#include <optional>

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
        // 输入事件分派（dispatch 按类型分发）
        auto handle_key(const device::KeyEvent& e) const -> void;
        auto handle_char(const device::CharEvent& e) const -> void;
        auto handle_ime(const platform::ImeCompositionEvent& e) const -> void;
        auto handle_mouse_move(const device::MouseMoveEvent& e) const -> void;
        auto handle_mouse_button(const device::MouseButtonEvent& e) const -> void;
        auto handle_mouse_wheel(const device::MouseWheelEvent& e) const -> void;
        auto handle_resize(const platform::ResizeEvent& e) const -> void;
        auto handle_dpi_change(const platform::DpiChangeEvent& e) const -> void;
        auto handle_theme_change(const platform::ThemeChangedEvent& e) const -> void;
        auto handle_destroy() const -> void;

        // 共享辅助
        [[nodiscard]] auto hit_target(const device::Mouse& mouse) const -> std::optional<std::shared_ptr<widget::Widget>>;
        [[nodiscard]] auto focused_target() const -> std::shared_ptr<widget::Widget>;
        [[nodiscard]] auto try_tab_navigate(const std::shared_ptr<Context>& context, const device::KeyEvent& e) const -> bool;

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
