#ifdef _WIN32
#include "DirectX11.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace neko::backend {
    auto DirectX11::draw_rect_fill(const Vec4I rect, const Color color) const -> void {
        if (ctx_ == nullptr || cbuffer_ == nullptr) {
            return;
        }
        struct RectData {
            float r_x, r_y, r_w, r_h;
            float c_r, c_g, c_b, c_a;
            float s_w, s_h;
            float p0, p1;
        } const data{
            .r_x = static_cast<float>(rect.x),
            .r_y = static_cast<float>(rect.y),
            .r_w = static_cast<float>(rect.z),
            .r_h = static_cast<float>(rect.w),
            .c_r = color.r() / 255.0F,
            .c_g = color.g() / 255.0F,
            .c_b = color.b() / 255.0F,
            .c_a = color.a() / 255.0F,
            .s_w = static_cast<float>(size_.x),
            .s_h = static_cast<float>(size_.y),
        };

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx_->Map(cbuffer_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            return;
        }
        std::memcpy(mapped.pData, &data, sizeof(data));
        ctx_->Unmap(cbuffer_, 0);
        ctx_->Draw(6, 0);
    }

    auto DirectX11::draw_rect(const Vec4I rect, const Color color, const int thickness) const -> void {
        draw_rect_fill({.x = rect.x, .y = rect.y, .z = rect.z, .w = thickness}, color);
        draw_rect_fill({.x = rect.x, .y = rect.y + rect.w - thickness, .z = rect.z, .w = thickness}, color);
        draw_rect_fill({.x = rect.x, .y = rect.y + thickness, .z = thickness, .w = rect.w - thickness * 2}, color);
        draw_rect_fill({.x = rect.x + rect.z - thickness, .y = rect.y + thickness, .z = thickness, .w = rect.w - thickness * 2}, color);
    }

    auto DirectX11::draw_line(const Vec2I from, const Vec2I to, const Color color, const int thickness) const -> void {
        const Vec2I d = to - from;
        if (std::abs(d.x) >= std::abs(d.y)) {
            draw_rect_fill({.x = std::min(from.x, to.x), .y = from.y - thickness / 2, .z = std::abs(d.x) + thickness, .w = thickness}, color);
        } else {
            draw_rect_fill({.x = from.x - thickness / 2, .y = std::min(from.y, to.y), .z = thickness, .w = std::abs(d.y) + thickness}, color);
        }
    }

    auto DirectX11::draw_circle_fill(const Vec2I center, const int radius, const Color color) const -> void {
        draw_rect_fill({.x = center.x - radius, .y = center.y - radius, .z = radius * 2, .w = radius * 2}, color);
    }

    auto DirectX11::draw_text(const std::string_view text, const Vec2I pos, const Color color, const float font_size) -> void {
        if (ctx_ == nullptr || text_cb_ == nullptr || ascii_atlases_[0].srv == nullptr || cjk_atlas_.srv == nullptr) {
            return;
        }
        if (text.empty()) {
            return;
        }

        // 目标像素大小决定图集档位与缩放；字号非法时钳制到 1px 避免零/负缩放
        const float target_px = std::max(font_size, 1.0F) * dpi_scale_;
        size_t ascii_atlas_idx = 0;
        for (size_t i = 0; i < FONT_SIZES.size(); i++) {
            if (target_px >= FONT_SIZES[i]) {
                ascii_atlas_idx = i;
            }
        }
        const FontAtlas& ascii_atlas = ascii_atlases_[ascii_atlas_idx];
        const float ascii_glyph_scale = target_px / ascii_atlas.font_size;
        const float cjk_glyph_scale = target_px / cjk_atlas_.font_size;

        ctx_->VSSetShader(text_vs_, nullptr, 0);
        ctx_->PSSetShader(text_ps_, nullptr, 0);
        ctx_->PSSetSamplers(0, 1, &font_sampler_);
        ctx_->OMSetBlendState(bs_text_, nullptr, 0xFFFFFFFF);

        TextCB cb{};
        cb.c_r = color.r() / 255.0F;
        cb.c_g = color.g() / 255.0F;
        cb.c_b = color.b() / 255.0F;
        cb.c_a = color.a() / 255.0F;
        cb.s_w = static_cast<float>(size_.x);
        cb.s_h = static_cast<float>(size_.y);

        // 笔位置以物理像素维护，逐字形换算到图集档位单位供 stbtt 使用（advance 随档位缩放）
        float pen_x = static_cast<float>(pos.x) * dpi_scale_;
        float pen_y = static_cast<float>(pos.y) * dpi_scale_;
        ID3D11ShaderResourceView* bound_srv = nullptr;

        for (size_t i = 0; i < text.size();) {
            const auto u8 = static_cast<unsigned char>(text[i]);
            int cp;
            size_t seq_len;
            if (u8 < 0x80) {
                cp = u8;
                seq_len = 1;
            } else if ((u8 & 0xE0) == 0xC0) {
                cp = u8 & 0x1F;
                seq_len = 2;
            } else if ((u8 & 0xF0) == 0xE0) {
                cp = u8 & 0x0F;
                seq_len = 3;
            } else if ((u8 & 0xF8) == 0xF0) {
                cp = u8 & 0x07;
                seq_len = 4;
            } else {
                i++;
                continue;
            }
            if (i + seq_len > text.size()) {
                break;
            }
            for (size_t j = 1; j < seq_len; j++) {
                cp = cp << 6 | (static_cast<unsigned char>(text[i + j]) & 0x3F);
            }
            i += seq_len;

            // 码点分类：ASCII/全角走多档图集，CJK 走单档；范围外字形直接跳过
            const FontAtlas* atlas = nullptr;
            float glyph_scale = 0.0F;
            int char_idx = 0;
            if (cp >= FONT_FIRST && cp < FONT_FIRST + FONT_COUNT) {
                atlas = &ascii_atlas;
                glyph_scale = ascii_glyph_scale;
                char_idx = cp - FONT_FIRST;
            } else if (cp >= FW_PUNCT_FIRST && cp <= FW_PUNCT_LAST) {
                atlas = &ascii_atlas;
                glyph_scale = ascii_glyph_scale;
                char_idx = FONT_COUNT + (cp - FW_PUNCT_FIRST);
            } else if (cp >= CJK_FIRST && cp <= CJK_LAST) {
                atlas = &cjk_atlas_;
                glyph_scale = cjk_glyph_scale;
                char_idx = cp - CJK_FIRST;
            }
            if (atlas == nullptr || static_cast<size_t>(char_idx) >= atlas->chars.size()) {
                continue;
            }

            float local_x = pen_x / glyph_scale;
            float local_y = pen_y / glyph_scale;
            stbtt_aligned_quad q{};
            stbtt_GetPackedQuad(atlas->chars.data(), atlas->width, atlas->height, char_idx, &local_x, &local_y, &q, 0);
            pen_x = local_x * glyph_scale;
            pen_y = local_y * glyph_scale;

            // 屏幕空间像素对齐：起点取整到物理像素，宽高保持浮点避免累积偏移
            cb.r_x = std::floor(q.x0 * glyph_scale + 0.5F);
            cb.r_y = std::floor(q.y0 * glyph_scale + 0.5F);
            cb.r_w = (q.x1 - q.x0) * glyph_scale;
            cb.r_h = (q.y1 - q.y0) * glyph_scale;
            cb.uv_u = q.s0;
            cb.uv_v = q.t0;
            cb.uv_w = q.s1 - q.s0;
            cb.uv_h = q.t1 - q.t0;

            if (atlas->srv != bound_srv) {
                bound_srv = atlas->srv;
                ctx_->PSSetShaderResources(0, 1, &bound_srv);
            }

            D3D11_MAPPED_SUBRESOURCE mapped{};
            if (FAILED(ctx_->Map(text_cb_, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                break;
            }
            std::memcpy(mapped.pData, &cb, sizeof(cb));
            ctx_->Unmap(text_cb_, 0);

            ctx_->VSSetConstantBuffers(0, 1, &text_cb_);
            ctx_->Draw(6, 0);
        }

        ctx_->VSSetShader(vs_, nullptr, 0);
        ctx_->PSSetShader(ps_, nullptr, 0);
        ctx_->PSSetShaderResources(0, 0, nullptr);
        ctx_->VSSetConstantBuffers(0, 1, &cbuffer_);
        ctx_->OMSetBlendState(bs_opaque_, nullptr, 0xFFFFFFFF);
    }
} // namespace neko::backend
#endif
