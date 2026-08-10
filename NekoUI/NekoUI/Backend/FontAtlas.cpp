// 2026-08-10 12:53:30

#ifdef _WIN32
#include "FontAtlas.hpp"

#include <fstream>
#include <print>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace neko::backend {
    FontAtlas::FontAtlas(const Surface& surface) :
        surface_{surface} {
        init_font();
    }

    FontAtlas::~FontAtlas() {
        release_font_resources();
        if (font_sampler_ != nullptr) {
            font_sampler_->Release();
        }
    }

    auto FontAtlas::query(const int codepoint, const float target_px, const Glyph*& out, int& char_idx, float& glyph_scale) const -> bool {
        const Atlas* atlas = nullptr;
        if (codepoint >= FONT_FIRST && codepoint < FONT_FIRST + FONT_COUNT) {
            // ASCII：选 ≤ 目标像素的最大档位
            size_t idx = 0;
            for (size_t i = 0; i < FONT_SIZES.size(); i++) {
                if (target_px >= FONT_SIZES[i]) {
                    idx = i;
                }
            }
            atlas = &ascii_atlases_[idx];
            char_idx = codepoint - FONT_FIRST;
        } else if (codepoint >= FW_PUNCT_FIRST && codepoint <= FW_PUNCT_LAST) {
            size_t idx = 0;
            for (size_t i = 0; i < FONT_SIZES.size(); i++) {
                if (target_px >= FONT_SIZES[i]) {
                    idx = i;
                }
            }
            atlas = &ascii_atlases_[idx];
            char_idx = FONT_COUNT + (codepoint - FW_PUNCT_FIRST);
        } else if (codepoint >= CJK_FIRST && codepoint <= CJK_LAST) {
            atlas = &cjk_atlas_;
            char_idx = codepoint - CJK_FIRST;
        } else {
            return false;
        }
        if (static_cast<size_t>(char_idx) >= atlas->chars.size()) {
            return false;
        }

        glyph_cache_.chars = atlas->chars.data();
        glyph_cache_.srv = atlas->srv;
        glyph_cache_.width = atlas->width;
        glyph_cache_.height = atlas->height;
        glyph_cache_.font_size = atlas->font_size;
        out = &glyph_cache_;
        glyph_scale = target_px / atlas->font_size;
        return true;
    }

    auto FontAtlas::sampler() const -> ID3D11SamplerState* {
        return font_sampler_;
    }

    auto FontAtlas::init_font() -> bool {
        std::vector<unsigned char> font_data;
        for (const auto& path : {L"C:\\Windows\\Fonts\\msyh.ttc", L"C:\\Windows\\Fonts\\segoeui.ttf"}) {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                continue;
            }
            const auto len = file.tellg();
            file.seekg(0);
            font_data.resize(len);
            file.read(reinterpret_cast<char*>(font_data.data()), len);
            if (file.gcount() == len) {
                break;
            }
            font_data.clear();
        }
        if (font_data.empty()) {
            return false;
        }

        std::vector<unsigned char> bitmap;

        // 3 档 ASCII + 全角标点图集（16/24/32px，1024²，每档 336 字形）
        constexpr int ASCII_CHAR_COUNT = FONT_COUNT + FW_PUNCT_COUNT;
        for (size_t i = 0; i < FONT_SIZES.size(); i++) {
            Atlas& atlas = ascii_atlases_[i];
            atlas.font_size = FONT_SIZES[i];
            atlas.chars.resize(ASCII_CHAR_COUNT);
            bitmap.assign(static_cast<size_t>(ASCII_ATLAS) * ASCII_ATLAS, 0);

            stbtt_pack_context pc{};
            stbtt_PackBegin(&pc, bitmap.data(), ASCII_ATLAS, ASCII_ATLAS, 0, 1, nullptr);
            stbtt_PackSetOversampling(&pc, 2, 2);
            std::array<stbtt_pack_range, 2> ranges{};
            ranges[0].font_size = atlas.font_size;
            ranges[0].first_unicode_codepoint_in_range = FONT_FIRST;
            ranges[0].num_chars = FONT_COUNT;
            ranges[0].chardata_for_range = atlas.chars.data();
            ranges[1].font_size = atlas.font_size;
            ranges[1].first_unicode_codepoint_in_range = FW_PUNCT_FIRST;
            ranges[1].num_chars = FW_PUNCT_COUNT;
            ranges[1].chardata_for_range = atlas.chars.data() + FONT_COUNT;
            const int packed = stbtt_PackFontRanges(&pc, font_data.data(), 0, ranges.data(), ranges.size());
            stbtt_PackEnd(&pc);
            if (packed == 0) {
                std::println(stderr, "[NekoUI] Font atlas pack failed: {}px {}x{} too small", atlas.font_size, ASCII_ATLAS, ASCII_ATLAS);
                release_font_resources();
                return false;
            }
            if (!create_atlas_texture(atlas, bitmap, ASCII_ATLAS, ASCII_ATLAS, surface_)) {
                std::println(stderr, "[NekoUI] Font atlas texture create failed: {}px", atlas.font_size);
                release_font_resources();
                return false;
            }
        }

        // CJK 图集（16px，4096²，20992 字形单档）
        cjk_atlas_.font_size = FONT_SIZES[0];
        cjk_atlas_.chars.resize(CJK_LAST - CJK_FIRST + 1);
        bitmap.assign(static_cast<size_t>(CJK_ATLAS) * CJK_ATLAS, 0);

        stbtt_pack_context pc{};
        stbtt_PackBegin(&pc, bitmap.data(), CJK_ATLAS, CJK_ATLAS, 0, 1, nullptr);
        stbtt_PackSetOversampling(&pc, 2, 2);
        std::array<stbtt_pack_range, 1> ranges{};
        ranges[0].font_size = cjk_atlas_.font_size;
        ranges[0].first_unicode_codepoint_in_range = CJK_FIRST;
        ranges[0].num_chars = CJK_LAST - CJK_FIRST + 1;
        ranges[0].chardata_for_range = cjk_atlas_.chars.data();
        const int packed = stbtt_PackFontRanges(&pc, font_data.data(), 0, ranges.data(), ranges.size());
        stbtt_PackEnd(&pc);
        if (packed == 0) {
            std::println(stderr, "[NekoUI] CJK atlas pack failed: {}x{} too small", CJK_ATLAS, CJK_ATLAS);
            release_font_resources();
            return false;
        }
        if (!create_atlas_texture(cjk_atlas_, bitmap, CJK_ATLAS, CJK_ATLAS, surface_)) {
            std::println(stderr, "[NekoUI] CJK atlas texture create failed");
            release_font_resources();
            return false;
        }

        std::println(stderr, "{} font atlases baked (ASCII {}px x3 + CJK {}px)", 3, FONT_SIZES[0], cjk_atlas_.font_size);

        // 图集 MipLevels=1，LINEAR 对 MIN/MAG 生效；放大时平滑插值消除像素化
        D3D11_SAMPLER_DESC sm{};
        sm.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        sm.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sm.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        std::println(stderr, "[diag] sm fields: filter={} addrU={} addrV={} addrW={} maxAniso={} minLOD={} maxLOD={} comp={} size={}",
                     static_cast<int>(sm.Filter), static_cast<int>(sm.AddressU), static_cast<int>(sm.AddressV),
                     static_cast<int>(sm.AddressW), sm.MaxAnisotropy, sm.MinLOD, sm.MaxLOD,
                     static_cast<int>(sm.ComparisonFunc), sizeof(sm));
        const HRESULT sampler_hr = surface_.device()->CreateSamplerState(&sm, &font_sampler_);
        std::println(stderr, "[diag] sampler create: device={} hr={:#010X} result={}", surface_.device() != nullptr, static_cast<unsigned int>(sampler_hr), font_sampler_ != nullptr);
        return true;
    }

    auto FontAtlas::create_atlas_texture(Atlas& atlas, const std::vector<unsigned char>& bitmap, const int width, const int height, const Surface& surface) -> bool {
        atlas.width = width;
        atlas.height = height;
        ID3D11Texture2D* tex{};
        D3D11_TEXTURE2D_DESC td{};
        td.Width = static_cast<UINT>(width);
        td.Height = static_cast<UINT>(height);
        td.Format = DXGI_FORMAT_A8_UNORM;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = bitmap.data();
        sd.SysMemPitch = static_cast<UINT>(width);
        if (FAILED(surface.device()->CreateTexture2D(&td, &sd, &tex))) {
            return false;
        }
        const bool ok = SUCCEEDED(surface.device()->CreateShaderResourceView(tex, nullptr, &atlas.srv));
        atlas.texture = tex;
        return ok;
    }

    auto FontAtlas::release_font_resources() -> void {
        for (auto& atlas : ascii_atlases_) {
            if (atlas.srv != nullptr) {
                atlas.srv->Release();
            }
            if (atlas.texture != nullptr) {
                atlas.texture->Release();
            }
            atlas.srv = nullptr;
            atlas.texture = nullptr;
        }
        if (cjk_atlas_.srv != nullptr) {
            cjk_atlas_.srv->Release();
        }
        if (cjk_atlas_.texture != nullptr) {
            cjk_atlas_.texture->Release();
        }
        cjk_atlas_.srv = nullptr;
        cjk_atlas_.texture = nullptr;
    }
} // namespace neko::backend
#endif
