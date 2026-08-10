// 2026-08-10 12:53:35

#pragma once
#ifdef _WIN32
#include <d3d11.h>

#include <array>
#include <vector>

#include "Surface.hpp"
#include "stb_truetype.h"

namespace neko::backend {
    class FontAtlas final {
    public:
        explicit FontAtlas(const Surface& surface);
        ~FontAtlas();

        FontAtlas(const FontAtlas&) = delete;
        auto operator=(const FontAtlas&) -> FontAtlas& = delete;

        struct Glyph {
            const stbtt_packedchar* chars = nullptr;
            ID3D11ShaderResourceView* srv = nullptr;
            int width = 0;
            int height = 0;
            float font_size = 0.0F;
        };

        [[nodiscard]] auto query(int codepoint, float target_px, const Glyph*& out, int& char_idx, float& glyph_scale) const -> bool;
        [[nodiscard]] auto sampler() const -> ID3D11SamplerState*;
    private:
        static constexpr int ASCII_ATLAS = 1024;
        static constexpr int CJK_ATLAS = 4096;
        static constexpr std::array<float, 3> FONT_SIZES = {16.0F, 24.0F, 32.0F};
        static constexpr int FONT_FIRST = 32;
        static constexpr int FONT_COUNT = 96;
        static constexpr int CJK_FIRST = 0x4E00;
        static constexpr int CJK_LAST = 0x9FFF;
        static constexpr int FW_PUNCT_FIRST = 0xFF00;
        static constexpr int FW_PUNCT_LAST = 0xFFEF;
        static constexpr int FW_PUNCT_COUNT = FW_PUNCT_LAST - FW_PUNCT_FIRST + 1;

        struct Atlas {
            ID3D11Texture2D* texture = nullptr;
            ID3D11ShaderResourceView* srv = nullptr;
            std::vector<stbtt_packedchar> chars;
            float font_size = 0.0F;
            int width = 0;
            int height = 0;
        };

        const Surface& surface_;
        std::array<Atlas, 3> ascii_atlases_{};
        Atlas cjk_atlas_{};
        ID3D11SamplerState* font_sampler_{};
        // query 输出缓冲（跨调用会覆盖，调用方须立即消费返回的 Glyph）
        mutable Glyph glyph_cache_{};

        auto init_font() -> bool;
        auto create_atlas_texture(Atlas& atlas, const std::vector<unsigned char>& bitmap, int width, int height, const Surface& surface) -> bool;
        auto release_font_resources() -> void;
    };
}
#endif
