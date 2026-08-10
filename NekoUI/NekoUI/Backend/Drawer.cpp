// 2026-08-10

#ifdef _WIN32
#include "Drawer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "stb_truetype.h"

namespace neko::backend {
    Drawer::Drawer(const Surface& surface, const Pipeline& pipeline, const FontAtlas& fonts) :
        surface_{surface}, pipeline_{pipeline}, fonts_{fonts} {
    }

    auto Drawer::draw_rect_fill(const Vec4I rect, const Color color) const -> void {
        ID3D11DeviceContext* ctx = surface_.context();
        if (ctx == nullptr || pipeline_.rect_cbuffer() == nullptr) {
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
            .s_w = static_cast<float>(surface_.size().x),
            .s_h = static_cast<float>(surface_.size().y),
        };

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx->Map(pipeline_.rect_cbuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            return;
        }
        std::memcpy(mapped.pData, &data, sizeof(data));
        ctx->Unmap(pipeline_.rect_cbuffer(), 0);
        ctx->Draw(6, 0);
    }

    auto Drawer::draw_rect(const Vec4I rect, const Color color, const int thickness) const -> void {
        draw_rect_fill({.x = rect.x, .y = rect.y, .z = rect.z, .w = thickness}, color);
        draw_rect_fill({.x = rect.x, .y = rect.y + rect.w - thickness, .z = rect.z, .w = thickness}, color);
        draw_rect_fill({.x = rect.x, .y = rect.y + thickness, .z = thickness, .w = rect.w - thickness * 2}, color);
        draw_rect_fill({.x = rect.x + rect.z - thickness, .y = rect.y + thickness, .z = thickness, .w = rect.w - thickness * 2}, color);
    }

    auto Drawer::draw_line(const Vec2I from, const Vec2I to, const Color color, const int thickness) const -> void {
        const Vec2I d = to - from;
        if (std::abs(d.x) >= std::abs(d.y)) {
            draw_rect_fill({.x = std::min(from.x, to.x), .y = from.y - thickness / 2, .z = std::abs(d.x) + thickness, .w = thickness}, color);
        } else {
            draw_rect_fill({.x = from.x - thickness / 2, .y = std::min(from.y, to.y), .z = thickness, .w = std::abs(d.y) + thickness}, color);
        }
    }

    auto Drawer::draw_circle_fill(const Vec2I center, const int radius, const Color color) const -> void {
        draw_rect_fill({.x = center.x - radius, .y = center.y - radius, .z = radius * 2, .w = radius * 2}, color);
    }

    auto Drawer::draw_text(const std::string_view text, const Vec2I pos, const Color color, const float font_size) const -> void {
        ID3D11DeviceContext* ctx = surface_.context();
        static bool diag = false;
        if (!diag) {
            diag = true;
            std::println(stderr, "[diag] draw_text: '{}' pos=({},{}) font={} | ctx={} tcb={} sampler={} tvs={} tps={} tblend={} dpi={}",
                         text, pos.x, pos.y, font_size,
                         ctx != nullptr, pipeline_.text_cbuffer() != nullptr, fonts_.sampler() != nullptr,
                         pipeline_.text_vs() != nullptr, pipeline_.text_ps() != nullptr, pipeline_.text_blend() != nullptr,
                         surface_.get_dpi_scale());
        }
        if (ctx == nullptr || pipeline_.text_cbuffer() == nullptr || fonts_.sampler() == nullptr) {
            return;
        }
        if (text.empty()) {
            return;
        }

        // 目标像素大小决定图集档位与缩放；字号非法时钳制到 1px 避免零/负缩放
        const float dpi_scale = surface_.get_dpi_scale();
        const float target_px = std::max(font_size, 1.0F) * dpi_scale;

        ctx->VSSetShader(pipeline_.text_vs(), nullptr, 0);
        ctx->PSSetShader(pipeline_.text_ps(), nullptr, 0);
        ID3D11SamplerState* sampler = fonts_.sampler();
        ctx->PSSetSamplers(0, 1, &sampler);
        ctx->OMSetBlendState(pipeline_.text_blend(), nullptr, 0xFFFFFFFF);

        Pipeline::TextCB cb{};
        cb.c_r = color.r() / 255.0F;
        cb.c_g = color.g() / 255.0F;
        cb.c_b = color.b() / 255.0F;
        cb.c_a = color.a() / 255.0F;
        cb.s_w = static_cast<float>(surface_.size().x);
        cb.s_h = static_cast<float>(surface_.size().y);

        // 笔位置以物理像素维护，逐字形换算到图集档位单位供 stbtt 使用（advance 随档位缩放）
        float pen_x = static_cast<float>(pos.x) * dpi_scale;
        float pen_y = static_cast<float>(pos.y) * dpi_scale;
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
            const FontAtlas::Glyph* glyph = nullptr;
            int char_idx = 0;
            float glyph_scale = 0.0F;
            if (!fonts_.query(cp, target_px, glyph, char_idx, glyph_scale)) {
                continue;
            }
            static bool diag2 = false;
            if (!diag2) {
                diag2 = true;
                std::println(stderr, "[diag] query: cp={:#x} idx={} scale={} srv={} chars={} atlas=({}x{})", cp, char_idx, glyph_scale, glyph->srv != nullptr, glyph->chars != nullptr, glyph->width, glyph->height);
            }

            float local_x = pen_x / glyph_scale;
            float local_y = pen_y / glyph_scale;
            stbtt_aligned_quad q{};
            stbtt_GetPackedQuad(glyph->chars, glyph->width, glyph->height, char_idx, &local_x, &local_y, &q, 0);
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

            if (glyph->srv != bound_srv) {
                bound_srv = glyph->srv;
                ctx->PSSetShaderResources(0, 1, &bound_srv);
            }

            D3D11_MAPPED_SUBRESOURCE mapped{};
            ID3D11Buffer* text_cb = pipeline_.text_cbuffer();
            if (FAILED(ctx->Map(text_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
                break;
            }
            std::memcpy(mapped.pData, &cb, sizeof(cb));
            ctx->Unmap(text_cb, 0);

            ctx->VSSetConstantBuffers(0, 1, &text_cb);
            ctx->Draw(6, 0);
        }

        pipeline_.bind_default();
        ctx->PSSetShaderResources(0, 0, nullptr);
    }
} // namespace neko::backend
#endif
