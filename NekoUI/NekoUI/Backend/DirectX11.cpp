// 2026-08-10 10:18:55

#ifdef _WIN32
#include "DirectX11.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <fstream>
#include <print>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

static constexpr auto shader_src = R"(
cbuffer RectData : register(b0) {
    float4 rect;
    float4 color;
    float2 screen_size;
    float2 _padding;
};

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
};

VSOutput vs_main(uint vid : SV_VertexID) {
    // T1(vid 0-2): TL(0,0), TR(1,0), BL(0,1)
    // T2(vid 3-5): BL(0,1), BR(1,1), TR(1,0)
    const uint u = (vid == 1 || vid >= 4) ? 1 : 0;
    const uint v = (vid >= 2 && vid != 5) ? 1 : 0;
    const float2 pos = rect.xy + float2(float(u), float(v)) * rect.zw;
    float2 clip = pos / screen_size * 2.0 - 1.0;
    clip.y = -clip.y;

    VSOutput o;
    o.pos = float4(clip, 0, 1);
    o.col = color;
    return o;
}

float4 ps_main(VSOutput input) : SV_TARGET {
    return input.col;
}
)";

static constexpr auto text_shader_src = R"(
cbuffer TextData : register(b0) {
    float4 rect;
    float4 color;
    float2 screen_size;
    float2 uv_offset;
    float2 uv_size;
    float2 _padding;
};

Texture2D font_tex : register(t0);
SamplerState font_sam : register(s0);

struct VSOutput {
    float4 pos : SV_POSITION;
    float4 col : COLOR;
    float2 uv  : TEXCOORD;
};

VSOutput vs_main(uint vid : SV_VertexID) {
    // T1(vid 0-2): TL(0,0), TR(1,0), BL(0,1)
    // T2(vid 3-5): BL(0,1), BR(1,1), TR(1,0)
    const uint u = (vid == 1 || vid >= 4) ? 1 : 0;
    const uint v = (vid >= 2 && vid != 5) ? 1 : 0;
    const float2 pos = rect.xy + float2(float(u), float(v)) * rect.zw;
    float2 clip = pos / screen_size * 2.0 - 1.0;
    clip.y = -clip.y;

    VSOutput o;
    o.pos = float4(clip, 0, 1);
    o.col = color;
    o.uv = uv_offset + float2(float(u), float(v)) * uv_size;
    return o;
}

float4 ps_main(VSOutput input) : SV_TARGET {
    float alpha = font_tex.Sample(font_sam, input.uv).a;
    return float4(input.col.rgb * alpha, input.col.a * alpha);
}
)";

namespace {
    auto compile_shader(const char* source, const char* entry, const char* target, ID3DBlob** blob) -> bool {
        ID3DBlob* error{};
        const HRESULT hr = D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, blob, &error);
        if (FAILED(hr)) {
            if (error != nullptr) {
                std::println(stderr, "{}", static_cast<const char*>(error->GetBufferPointer()));
                error->Release();
            }
            return false;
        }
        return true;
    }
}

namespace neko::backend {
    DirectX11::DirectX11(const HWND hwnd) :
        hwnd_{hwnd} {
        init_device();
        init_swap_chain(hwnd);
        init_shaders();
        init_states();
        init_font();
    }

    DirectX11::~DirectX11() {
        release_font_resources();
        if (font_sampler_ != nullptr) {
            font_sampler_->Release();
        }
        if (text_cb_ != nullptr) {
            text_cb_->Release();
        }
        if (text_ps_ != nullptr) {
            text_ps_->Release();
        }
        if (text_vs_ != nullptr) {
            text_vs_->Release();
        }
        if (bs_text_ != nullptr) {
            bs_text_->Release();
        }
        if (bs_opaque_ != nullptr) {
            bs_opaque_->Release();
        }
        if (rs_ != nullptr) {
            rs_->Release();
        }
        if (cbuffer_ != nullptr) {
            cbuffer_->Release();
        }
        if (layout_ != nullptr) {
            layout_->Release();
        }
        if (ps_ != nullptr) {
            ps_->Release();
        }
        if (vs_ != nullptr) {
            vs_->Release();
        }
        if (rtv_ != nullptr) {
            rtv_->Release();
        }
        if (swap_chain_ != nullptr) {
            swap_chain_->Release();
        }
        if (ctx_ != nullptr) {
            ctx_->Release();
        }
        if (device_ != nullptr) {
            device_->Release();
        }
    }

    auto DirectX11::resize(const Vec2I new_size) -> void {
        if (size_ == new_size) {
            return;
        }

        if (rtv_ != nullptr) {
            rtv_->Release();
            rtv_ = nullptr;
        }
        if (swap_chain_ != nullptr) {
            swap_chain_->ResizeBuffers(0, static_cast<UINT>(std::max(new_size.x, 1)), static_cast<UINT>(std::max(new_size.y, 1)), DXGI_FORMAT_UNKNOWN, 0);
            ID3D11Texture2D* bb{};
            if (SUCCEEDED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&bb)))) {
                device_->CreateRenderTargetView(bb, nullptr, &rtv_);
                bb->Release();
            }
        }
        size_ = new_size;
    }

    auto DirectX11::set_dpi(const unsigned int dpi) -> void {
        dpi_scale_ = static_cast<float>(dpi) / 96.0F;
    }

    auto DirectX11::get_dpi_scale() const -> float {
        return dpi_scale_;
    }

    auto DirectX11::begin() const -> void {
        static bool diag_begin = false;
        if (!diag_begin) {
            diag_begin = true;
            std::println(stderr, "[diag] begin: size=({},{}), rtv={}, vs={}, ps={}, rs={}, bs_opaque={}, bs_text={}, ctx={}",
                         size_.x, size_.y, rtv_ != nullptr, vs_ != nullptr, ps_ != nullptr, rs_ != nullptr, bs_opaque_ != nullptr, bs_text_ != nullptr, ctx_ != nullptr);
        }
        if (ctx_ == nullptr || rtv_ == nullptr) {
            return;
        }
        ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(size_.x);
        vp.Height = static_cast<float>(size_.y);
        vp.MaxDepth = 1.0F;
        ctx_->RSSetViewports(1, &vp);

        // EXPERIMENT-C: 红色 clear——验证交换链/clear 层是否工作
        constexpr std::array color{1.0F, 0.0F, 0.0F, 1.0F};
        ctx_->ClearRenderTargetView(rtv_, color.data());

        ctx_->RSSetState(rs_);
        ctx_->OMSetBlendState(bs_opaque_, nullptr, 0xFFFFFFFF);
        ctx_->IASetInputLayout(layout_);
        ctx_->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx_->VSSetShader(vs_, nullptr, 0);
        ctx_->PSSetShader(ps_, nullptr, 0);
        ctx_->VSSetConstantBuffers(0, 1, &cbuffer_);
    }

    auto DirectX11::end() const -> void {
        if (swap_chain_ == nullptr) {
            return;
        }
        const HRESULT hr = swap_chain_->Present(1, 0);
        static bool diag_present = false;
        if (!diag_present) {
            diag_present = true;
            std::println(stderr, "[diag] present hr={:#010X} ({})", static_cast<unsigned int>(hr), hr == S_OK ? "OK" : "FAIL");
        }
    }

    auto DirectX11::draw_rect_fill(const Vec4I rect, const Color color) const -> void {
        static int diag_rect = 0;
        if (diag_rect < 5) {
            diag_rect++;
            std::println(stderr, "[diag] rect#{}: ({},{},{},{}) color={:08X}", diag_rect, rect.x, rect.y, rect.z, rect.w, color.value);
        }
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
        static int diag_text = 0;
        if (diag_text < 3) {
            diag_text++;
            std::println(stderr, "[diag] text#{}: '{}' at ({},{}) size={} color={:08X} dpi={}", diag_text, text, pos.x, pos.y, font_size, color.value, dpi_scale_);
        }
        return; // EXPERIMENT-A: 临时禁用文字绘制——验证是否污染矩形状态
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

    auto DirectX11::get_client_size() const -> Vec2I {
        RECT client{};
        if (GetClientRect(hwnd_, &client) != 0) {
            return {.x = client.right - client.left, .y = client.bottom - client.top};
        }
        return {};
    }

    auto DirectX11::init_device() -> void {
        UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        #ifdef _DEBUG
        create_flags |= D3D11_CREATE_DEVICE_DEBUG;
        #endif
        D3D_FEATURE_LEVEL feature_level{};
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags, nullptr, 0, D3D11_SDK_VERSION, &device_, &feature_level, &ctx_))) {
            std::println(stderr, "[NekoUI] D3D11CreateDevice failed");
        }
    }

    auto DirectX11::init_swap_chain(const HWND hwnd) -> void {
        IDXGIFactory2* factory{};
        {
            IDXGIDevice1* dxgi_device{};
            IDXGIAdapter* adapter{};
            if (SUCCEEDED(device_->QueryInterface(&dxgi_device)) && SUCCEEDED(dxgi_device->GetAdapter(&adapter))) {
                adapter->GetParent(IID_PPV_ARGS(&factory));
                adapter->Release();
                dxgi_device->Release();
            }
        }
        if (factory == nullptr) {
            std::println(stderr, "[NekoUI] DXGI factory init failed");
            return;
        }

        RECT rc{};
        GetClientRect(hwnd, &rc);

        DXGI_SWAP_CHAIN_DESC1 sc_desc{};
        sc_desc.Width = static_cast<UINT>(rc.right > 0 ? rc.right : 1);
        sc_desc.Height = static_cast<UINT>(rc.bottom > 0 ? rc.bottom : 1);
        sc_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        sc_desc.SampleDesc.Count = 1;
        sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sc_desc.BufferCount = 2;
        sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

        if (FAILED(factory->CreateSwapChainForHwnd(device_, hwnd, &sc_desc, nullptr, nullptr, &swap_chain_))) {
            factory->Release();
            std::println(stderr, "[NekoUI] SwapChain init failed");
            return;
        }
        factory->Release();

        size_ = {.x = static_cast<int>(sc_desc.Width), .y = static_cast<int>(sc_desc.Height)};
        ID3D11Texture2D* back_buffer{};
        if (SUCCEEDED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
            device_->CreateRenderTargetView(back_buffer, nullptr, &rtv_);
            back_buffer->Release();
        }

        const UINT dpi = GetDpiForWindow(hwnd);
        dpi_scale_ = dpi > 0 ? static_cast<float>(dpi) / 96.0F : 1.0F;
    }

    auto DirectX11::init_shaders() -> bool {
        ID3DBlob* vs_blob{};
        ID3DBlob* ps_blob{};
        if (!compile_shader(shader_src, "vs_main", "vs_5_0", &vs_blob)) {
            return false;
        }
        if (!compile_shader(shader_src, "ps_main", "ps_5_0", &ps_blob)) {
            vs_blob->Release();
            return false;
        }
        device_->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vs_);
        device_->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &ps_);
        device_->CreateInputLayout(nullptr, 0, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &layout_);
        vs_blob->Release();
        ps_blob->Release();

        D3D11_BUFFER_DESC cb_desc{};
        cb_desc.ByteWidth = sizeof(float) * 12;
        cb_desc.Usage = D3D11_USAGE_DYNAMIC;
        cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device_->CreateBuffer(&cb_desc, nullptr, &cbuffer_);

        ID3DBlob* tvs_blob{};
        ID3DBlob* tps_blob{};
        if (compile_shader(text_shader_src, "vs_main", "vs_5_0", &tvs_blob) && compile_shader(text_shader_src, "ps_main", "ps_5_0", &tps_blob)) {
            device_->CreateVertexShader(tvs_blob->GetBufferPointer(), tvs_blob->GetBufferSize(), nullptr, &text_vs_);
            device_->CreatePixelShader(tps_blob->GetBufferPointer(), tps_blob->GetBufferSize(), nullptr, &text_ps_);
            tvs_blob->Release();
            tps_blob->Release();

            D3D11_BUFFER_DESC tcb_desc{};
            tcb_desc.ByteWidth = sizeof(TextCB);
            tcb_desc.Usage = D3D11_USAGE_DYNAMIC;
            tcb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            tcb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            device_->CreateBuffer(&tcb_desc, nullptr, &text_cb_);
        }
        return true;
    }

    auto DirectX11::init_states() -> bool {
        D3D11_RASTERIZER_DESC rs_desc;
        rs_desc.FillMode = D3D11_FILL_SOLID;
        rs_desc.CullMode = D3D11_CULL_NONE;
        rs_desc.DepthClipEnable = 1;
        device_->CreateRasterizerState(&rs_desc, &rs_);

        D3D11_BLEND_DESC bd;
        bd.IndependentBlendEnable = 0;
        auto& [BlendEnable, SrcBlend, DestBlend, BlendOp, SrcBlendAlpha, DestBlendAlpha, BlendOpAlpha, RenderTargetWriteMask] = bd.RenderTarget[0];
        RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        BlendEnable = 0;
        device_->CreateBlendState(&bd, &bs_opaque_);

        // 文本专用预乘混合：着色器输出已预乘 alpha，避免深色背景暗边 fringing
        BlendEnable = 1;
        SrcBlend = D3D11_BLEND_ONE;
        DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        BlendOp = D3D11_BLEND_OP_ADD;
        SrcBlendAlpha = D3D11_BLEND_ONE;
        DestBlendAlpha = D3D11_BLEND_ZERO;
        BlendOpAlpha = D3D11_BLEND_OP_ADD;
        device_->CreateBlendState(&bd, &bs_text_);
        return true;
    }

    auto DirectX11::create_atlas_texture(FontAtlas& atlas, const std::vector<unsigned char>& bitmap, const int width, const int height) -> bool {
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
        if (FAILED(device_->CreateTexture2D(&td, &sd, &tex))) {
            return false;
        }
        const bool ok = SUCCEEDED(device_->CreateShaderResourceView(tex, nullptr, &atlas.srv));
        atlas.texture = tex;
        return ok;
    }

    auto DirectX11::release_font_resources() -> void {
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

    auto DirectX11::init_font() -> bool {
        if (text_cb_ == nullptr) {
            return false;
        }

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
            FontAtlas& atlas = ascii_atlases_[i];
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
            if (!create_atlas_texture(atlas, bitmap, ASCII_ATLAS, ASCII_ATLAS)) {
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
        if (!create_atlas_texture(cjk_atlas_, bitmap, CJK_ATLAS, CJK_ATLAS)) {
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
        device_->CreateSamplerState(&sm, &font_sampler_);
        return true;
    }
} // namespace neko::backend
#endif
