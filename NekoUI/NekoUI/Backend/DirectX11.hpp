// 2026-08-10 12:53:11

#pragma once
#ifdef _WIN32
#include <Windows.h>

#include <string_view>

#include "../Type.hpp"
#include "Drawer.hpp"
#include "FontAtlas.hpp"
#include "Pipeline.hpp"
#include "Surface.hpp"

namespace neko::backend {
    using namespace neko::type;

    class DirectX11 final {
    public:
        explicit DirectX11(HWND hwnd);
        ~DirectX11() = default;

        DirectX11(const DirectX11&) = delete;
        auto operator=(const DirectX11&) -> DirectX11& = delete;
        DirectX11(DirectX11&&) = delete;
        auto operator=(DirectX11&&) -> DirectX11& = delete;

        auto resize(const Vec2I new_size) -> void {
            surface_.resize(new_size);
        }

        auto set_dpi(const unsigned int dpi) -> void {
            surface_.set_dpi(dpi);
        }

        [[nodiscard]] auto get_dpi_scale() const -> float {
            return surface_.get_dpi_scale();
        }

        auto begin() const -> void {
            surface_.begin_rt();
            pipeline_.bind_default();
        }

        auto end() const -> void {
            surface_.end();
        }

        auto draw_rect_fill(const Vec4I rect, const Color color) const -> void {
            drawer_.draw_rect_fill(rect, color);
        }

        auto draw_rect(const Vec4I rect, const Color color, const int thickness) const -> void {
            drawer_.draw_rect(rect, color, thickness);
        }

        auto draw_line(const Vec2I from, const Vec2I to, const Color color, const int thickness) const -> void {
            drawer_.draw_line(from, to, color, thickness);
        }

        auto draw_circle_fill(const Vec2I center, const int radius, const Color color) const -> void {
            drawer_.draw_circle_fill(center, radius, color);
        }

        auto draw_text(const std::string_view text, const Vec2I pos, const Color color, const float font_size = 16.0F) const -> void {
            drawer_.draw_text(text, pos, color, font_size);
        }

        [[nodiscard]] auto get_native_handle() const -> Handle {
            return surface_.native_handle();
        }

        [[nodiscard]] auto get_client_size() const -> Vec2I {
            return surface_.client_size();
        }
    private:
        Surface surface_;
        Pipeline pipeline_{surface_};
        FontAtlas fonts_{surface_};
        Drawer drawer_{surface_, pipeline_, fonts_};
    };
}
#endif
