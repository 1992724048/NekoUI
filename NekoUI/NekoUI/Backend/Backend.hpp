#pragma once
#include "../Window/Window.hpp"

#include <memory>

namespace neko::backend {
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

        virtual auto submit() -> void {}
        virtual auto draw_text() -> void {}
        virtual auto draw_line() -> void {}
        virtual auto draw_triangle() -> void {}
        virtual auto draw_rect() -> void {}
        virtual auto draw_rect_fill() -> void {}
        virtual auto draw_circle_fill() -> void {}
        virtual auto draw_image() -> void {}
    private:
        std::shared_ptr<window::Window> window;
    };
} // namespace neko::backend
