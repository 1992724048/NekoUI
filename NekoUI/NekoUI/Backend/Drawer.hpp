// 2026-08-10 12:53:27

#pragma once
#ifdef _WIN32
#include <string_view>

#include "../Type.hpp"
#include "FontAtlas.hpp"
#include "Pipeline.hpp"
#include "Surface.hpp"

namespace neko::backend {
    using namespace neko::type;

    class Drawer final {
    public:
        Drawer(const Surface& surface, const Pipeline& pipeline, const FontAtlas& fonts);
        auto draw_rect_fill(Vec4I rect, Color color) const -> void;
        auto draw_rect(Vec4I rect, Color color, int thickness) const -> void;
        auto draw_line(Vec2I from, Vec2I to, Color color, int thickness) const -> void;
        auto draw_circle_fill(Vec2I center, int radius, Color color) const -> void;
        auto draw_text(std::string_view text, Vec2I pos, Color color, float font_size = 16.0F) const -> void;
    private:
        const Surface& surface_;
        const Pipeline& pipeline_;
        const FontAtlas& fonts_;
    };
}
#endif
