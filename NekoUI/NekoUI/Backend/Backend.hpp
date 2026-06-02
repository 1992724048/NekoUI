#pragma once
#include "../Window/Window.hpp"

#include <memory>

namespace neko::backend {
    using namespace neko::type;

    /**
     * <summary>
     * 渲染后端
     * </summary>
     */
    class Backend {
    public:
        using Handle = void*;

        virtual ~Backend() = default;

        [[nodiscard]] auto get_window_handle() const -> Handle {
            if (window == nullptr) {
                return nullptr;
            }
            return window->get_handle();
        }

        virtual auto resize(Vec2<int> new_size) -> void {}

        virtual auto submit() -> void {}
        virtual auto draw_text() -> void {}
        virtual auto draw_line() -> void {}
        virtual auto draw_triangle() -> void {}
        virtual auto draw_rect(Vec4<int> range, Color rgba, int thickness) -> void {}
        virtual auto draw_rect_fill() -> void {}
        virtual auto draw_circle_fill() -> void {}
        virtual auto draw_image() -> void {}

        std::function<void()> render_callback;
    protected:
        Vec2<int> size{};
        std::shared_ptr<window::Window> window;
    };
} // namespace neko::backend
