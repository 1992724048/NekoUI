#ifdef _WIN32
#include "DirectX11.hpp"

#include <array>
#include <cstdio>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <fstream>
#include <print>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../stb_truetype.h"

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
    return float4(input.col.rgb, input.col.a * alpha);
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
        if (font_sampler_ != nullptr) {
            font_sampler_->Release();
        }
        if (font_srv_ != nullptr) {
            font_srv_->Release();
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
        if (bs_alpha_ != nullptr) {
            bs_alpha_->Release();
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
        if (ctx_ == nullptr || rtv_ == nullptr) {
            return;
        }
        ctx_->OMSetRenderTargets(1, &rtv_, nullptr);
        D3D11_VIEWPORT vp{};
        vp.Width = static_cast<float>(size_.x);
        vp.Height = static_cast<float>(size_.y);
        vp.MaxDepth = 1.0F;
        ctx_->RSSetViewports(1, &vp);

        constexpr std::array color{0.0F, 0.0F, 0.0F, 0.0F};
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
        swap_chain_->Present(1, 0);
    }

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
            .r_x = static_cast<float>(rect.x) * dpi_scale_,
            .r_y = static_cast<float>(rect.y) * dpi_scale_,
            .r_w = static_cast<float>(rect.z) * dpi_scale_,
            .r_h = static_cast<float>(rect.w) * dpi_scale_,
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
        draw_rect_fill({rect.x, rect.y, rect.z, thickness}, color);
        draw_rect_fill({rect.x, rect.y + rect.w - thickness, rect.z, thickness}, color);
        draw_rect_fill({rect.x, rect.y + thickness, thickness, rect.w - (thickness * 2)}, color);
        draw_rect_fill({rect.x + rect.z - thickness, rect.y + thickness, thickness, rect.w - (thickness * 2)}, color);
    }

    auto DirectX11::draw_line(const Vec2I from, const Vec2I to, const Color color, const int thickness) const -> void {
        const Vec2I d = to - from;
        if (std::abs(d.x) >= std::abs(d.y)) {
            draw_rect_fill({std::min(from.x, to.x), from.y - (thickness / 2), std::abs(d.x) + thickness, thickness}, color);
        } else {
            draw_rect_fill({from.x - (thickness / 2), std::min(from.y, to.y), thickness, std::abs(d.y) + thickness}, color);
        }
    }

    auto DirectX11::draw_circle_fill(const Vec2I center, const int radius, const Color color) const -> void {
        draw_rect_fill({center.x - radius, center.y - radius, radius * 2, radius * 2}, color);
    }

    auto DirectX11::draw_text(const std::string_view text, const Vec2I pos, const Color color, const float font_size) -> void {
        if (ctx_ == nullptr || text_cb_ == nullptr || font_srv_ == nullptr) {
            return;
        }

        const float scale = font_size / font_size_;

        ctx_->VSSetShader(text_vs_, nullptr, 0);
        ctx_->PSSetShader(text_ps_, nullptr, 0);
        ctx_->PSSetShaderResources(0, 1, &font_srv_);
        ctx_->PSSetSamplers(0, 1, &font_sampler_);
        ctx_->OMSetBlendState(bs_alpha_, nullptr, 0xFFFFFFFF);

        TextCB cb{};
        cb.c_r = color.r() / 255.0F;
        cb.c_g = color.g() / 255.0F;
        cb.c_b = color.b() / 255.0F;
        cb.c_a = color.a() / 255.0F;
        cb.s_w = static_cast<float>(size_.x);
        cb.s_h = static_cast<float>(size_.y);

        float ux = static_cast<float>(pos.x) / scale;
        float uy = static_cast<float>(pos.y) / scale;

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

            const stbtt_packedchar* bc = nullptr;
            int char_idx = 0;
            if (cp >= FONT_FIRST && cp < FONT_FIRST + FONT_COUNT) {
                bc = glyphs_.data();
                char_idx = cp - FONT_FIRST;
            } else {
                const auto it = cjk_glyphs_.find(cp);
                if (it != cjk_glyphs_.end()) {
                    bc = &it->second;
                    char_idx = 0;
                }
            }
            i += seq_len;
            if (bc == nullptr) {
                continue;
            }

            stbtt_aligned_quad q{};
            stbtt_GetPackedQuad(bc, ATLAS_W, ATLAS_H, char_idx, &ux, &uy, &q, 1);

            const float total = scale * dpi_scale_;
            cb.r_x = q.x0 * total;
            cb.r_y = q.y0 * total;
            cb.r_w = (q.x1 - q.x0) * total;
            cb.r_h = (q.y1 - q.y0) * total;
            cb.uv_u = q.s0;
            cb.uv_v = q.t0;
            cb.uv_w = q.s1 - q.s0;
            cb.uv_h = q.t1 - q.t0;

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

    auto DirectX11::init_device() -> void {
        UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        #ifdef _DEBUG
        create_flags |= D3D11_CREATE_DEVICE_DEBUG;
        #endif
        D3D_FEATURE_LEVEL feature_level{};
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags,
            nullptr, 0, D3D11_SDK_VERSION, &device_, &feature_level, &ctx_))) {
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

        size_ = {static_cast<int>(sc_desc.Width), static_cast<int>(sc_desc.Height)};
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

        BlendEnable = 1;
        SrcBlend = D3D11_BLEND_SRC_ALPHA;
        DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        BlendOp = D3D11_BLEND_OP_ADD;
        SrcBlendAlpha = D3D11_BLEND_ONE;
        DestBlendAlpha = D3D11_BLEND_ZERO;
        BlendOpAlpha = D3D11_BLEND_OP_ADD;
        device_->CreateBlendState(&bd, &bs_alpha_);
        return true;
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

        font_size_ = 16.0F;

        std::vector<unsigned char> bitmap(static_cast<size_t>(ATLAS_W) * ATLAS_H, 0);
        std::vector<stbtt_packedchar> cjk_output(CJK_LAST - CJK_FIRST + 1);

        stbtt_pack_context pc{};
        stbtt_PackBegin(&pc, bitmap.data(), ATLAS_W, ATLAS_H, 0, 1, nullptr);
        stbtt_PackSetOversampling(&pc, 2, 2);

        std::array<stbtt_pack_range, 2> ranges{};
        ranges[0].font_size = font_size_;
        ranges[0].first_unicode_codepoint_in_range = FONT_FIRST;
        ranges[0].num_chars = FONT_COUNT;
        ranges[0].chardata_for_range = glyphs_.data();

        ranges[1].font_size = font_size_;
        ranges[1].first_unicode_codepoint_in_range = CJK_FIRST;
        ranges[1].num_chars = CJK_LAST - CJK_FIRST + 1;
        ranges[1].chardata_for_range = cjk_output.data();

        stbtt_PackFontRanges(&pc, font_data.data(), 0, ranges.data(), 2);
        stbtt_PackEnd(&pc);

        for (int i = 0; i < CJK_LAST - CJK_FIRST + 1; i++) {
            const auto& g = cjk_output[i];
            if (g.x0 < g.x1 && g.y0 < g.y1) {
                cjk_glyphs_[CJK_FIRST + i] = g;
            }
        }

        std::println(stderr, "{} CJK glyphs baked", cjk_glyphs_.size());

        ID3D11Texture2D* tex{};
        D3D11_TEXTURE2D_DESC td{};
        td.Width = ATLAS_W;
        td.Height = ATLAS_H;
        td.Format = DXGI_FORMAT_A8_UNORM;
        td.MipLevels = 1;
        td.ArraySize = 1;
        td.SampleDesc.Count = 1;
        td.Usage = D3D11_USAGE_IMMUTABLE;
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sd{};
        sd.pSysMem = bitmap.data();
        sd.SysMemPitch = ATLAS_W;
        if (SUCCEEDED(device_->CreateTexture2D(&td, &sd, &tex))) {
            device_->CreateShaderResourceView(tex, nullptr, &font_srv_);
            tex->Release();
        }

        D3D11_SAMPLER_DESC sm;
        sm.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        sm.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sm.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        device_->CreateSamplerState(&sm, &font_sampler_);
        return true;
    }
} // namespace neko::backend
#endif
