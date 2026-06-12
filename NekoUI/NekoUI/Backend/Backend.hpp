#pragma once
#include "../Window/Window.hpp"

#include <memory>

namespace neko::backend {
    using namespace neko::type;

    //! @brief 渲染后端
    //! @note vulkan only
    class Backend final {
    public:
        Backend();
        ~Backend();

        auto get_handle() -> Handle;

        auto resize(Handle window_handle, Vec2<int> new_size) -> void;

        auto attach(const std::shared_ptr<window::Window>& window) -> bool;
        auto deattach(const std::shared_ptr<window::Window>& window) -> bool;

        auto submit(Handle window_handle) -> void;

        auto draw_text(Handle window_handle) -> void;
        auto draw_line(Handle window_handle) -> void;
        auto draw_triangle(Handle window_handle) -> void;
        auto draw_rect(Handle window_handle, Vec4<int> range, Color rgba, int thickness) -> void;
        auto draw_rect_fill(Handle window_handle) -> void;
        auto draw_circle_fill(Handle window_handle) -> void;
        auto draw_image(Handle window_handle) -> void;

        std::function<void()> render_callback;
    };
} // namespace neko::backend
