#pragma once
#include "../Window/Window.hpp"

#include <memory>

namespace neko::backend {
    using namespace neko::type;

    //! @brief 渲染后端
    class Backend {
    public:
        using Handle = void*;

        virtual ~Backend() = default;

        virtual auto get_handle() -> Handle {
            return nullptr;
        }

        virtual auto resize(Handle window_handle, Vec2<int> new_size) -> void {}

        virtual auto attach(const std::shared_ptr<window::Window>& window) -> bool {
            return false;
        }

        virtual auto submit(Handle window_handle) -> void {}

        virtual auto draw_text(Handle window_handle) -> void {}
        virtual auto draw_line(Handle window_handle) -> void {}
        virtual auto draw_triangle(Handle window_handle) -> void {}
        virtual auto draw_rect(Handle window_handle, Vec4<int> range, Color rgba, int thickness) -> void {}
        virtual auto draw_rect_fill(Handle window_handle) -> void {}
        virtual auto draw_circle_fill(Handle window_handle) -> void {}
        virtual auto draw_image(Handle window_handle) -> void {}

        std::function<void()> render_callback;
    };
} // namespace neko::backend
