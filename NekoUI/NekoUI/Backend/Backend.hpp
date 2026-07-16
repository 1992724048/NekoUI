#pragma once
#include <cstdint>
#include <string_view>

#include "../Type.hpp"

namespace neko::engine {
    class Engine;
}

namespace neko::backend {
    using namespace neko::type;

    class Backend {
    public:
        Backend() = default;
        virtual ~Backend() = default;
        Backend(const Backend&) = delete;
        auto operator=(const Backend&) -> Backend& = delete;

        virtual auto resize(Vec2I new_size) -> void = 0;
        virtual auto set_dpi(unsigned int dpi) -> void = 0;
        [[nodiscard]] virtual auto get_dpi_scale() const -> float = 0;

        virtual auto begin() const -> void = 0;
        virtual auto end() const -> void = 0;

        virtual auto draw_rect_fill(Vec4I rect, Color color) const -> void = 0;
        virtual auto draw_rect(Vec4I rect, Color color, int thickness) const -> void = 0;
        virtual auto draw_line(Vec2I from, Vec2I to, Color color, int thickness) const -> void = 0;
        virtual auto draw_circle_fill(Vec2I center, int radius, Color color) const -> void = 0;
        virtual auto draw_text(std::string_view text, Vec2I pos, Color color, float font_size = 16.0F) -> void = 0;

        [[nodiscard]] virtual auto get_native_handle() const -> void* = 0;
    };
} // namespace neko::backend
