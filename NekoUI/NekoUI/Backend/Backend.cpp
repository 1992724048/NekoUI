// 2026-07-02 03:12:05

#include "Backend.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <array>
#include <cstdio>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <fstream>
#include <print>
#include <vector>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

using namespace neko::backend;

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
} // namespace

Backend::Backend(const HWND hwnd) {
    if (!init_device(hwnd)) {
        return;
    }
    if (!init_shaders()) {
        return;
    }
    init_states();
    init_font();
}

Backend::~Backend() {
    if (font_sampler != nullptr) {
        font_sampler->Release();
    }
    if (font_srv != nullptr) {
        font_srv->Release();
    }
    if (text_cb != nullptr) {
        text_cb->Release();
    }
    if (text_ps != nullptr) {
        text_ps->Release();
    }
    if (text_vs != nullptr) {
        text_vs->Release();
    }
    if (bs_alpha != nullptr) {
        bs_alpha->Release();
    }
    if (bs_opaque != nullptr) {
        bs_opaque->Release();
    }
    if (rs != nullptr) {
        rs->Release();
    }
    if (cbuffer != nullptr) {
        cbuffer->Release();
    }
    if (layout != nullptr) {
        layout->Release();
    }
    if (ps != nullptr) {
        ps->Release();
    }
    if (vs != nullptr) {
        vs->Release();
    }
    if (rtv != nullptr) {
        rtv->Release();
    }
    if (swap_chain != nullptr) {
        swap_chain->Release();
    }
    if (ctx != nullptr) {
        ctx->Release();
    }
    if (device != nullptr) {
        device->Release();
    }
}

auto Backend::resize(const Vec2I new_size) -> void {
    if (size == new_size) {
        return;
    }

    if (rtv != nullptr) {
        rtv->Release();
        rtv = nullptr;
    }
    if (swap_chain != nullptr) {
        swap_chain->ResizeBuffers(0, static_cast<UINT>(std::max(new_size.x, 1)), static_cast<UINT>(std::max(new_size.y, 1)), DXGI_FORMAT_UNKNOWN, 0);
        ID3D11Texture2D* bb{};
        if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&bb)))) {
            device->CreateRenderTargetView(bb, nullptr, &rtv);
            bb->Release();
        }
    }
    size = new_size;
}

auto Backend::set_dpi(const UINT dpi) -> void {
    dpi_scale = static_cast<float>(dpi) / 96.0F;
}

auto Backend::begin() const -> void {
    if (ctx == nullptr || rtv == nullptr) {
        return;
    }
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(size.x);
    vp.Height = static_cast<float>(size.y);
    vp.MaxDepth = 1.0F;
    ctx->RSSetViewports(1, &vp);

    constexpr std::array color{0.0F, 0.0F, 0.0F, 0.0F};
    ctx->ClearRenderTargetView(rtv, color.data());

    ctx->RSSetState(rs);
    ctx->OMSetBlendState(bs_opaque, nullptr, 0xFFFFFFFF);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cbuffer);
}

auto Backend::end() const -> void {
    if (swap_chain == nullptr) {
        return;
    }
    swap_chain->Present(1, 0);
}

auto Backend::draw_rect_fill(const Vec4I rect, const Color color) const -> void {
    if (ctx == nullptr || cbuffer == nullptr) {
        return;
    }
    struct RectData {
        float r_x, r_y, r_w, r_h;
        float c_r, c_g, c_b, c_a;
        float s_w, s_h;
        float p0, p1;
    } const data{
        .r_x = static_cast<float>(rect.x) * dpi_scale,
        .r_y = static_cast<float>(rect.y) * dpi_scale,
        .r_w = static_cast<float>(rect.z) * dpi_scale,
        .r_h = static_cast<float>(rect.w) * dpi_scale,
        .c_r = color.r() / 255.0F,
        .c_g = color.g() / 255.0F,
        .c_b = color.b() / 255.0F,
        .c_a = color.a() / 255.0F,
        .s_w = static_cast<float>(size.x),
        .s_h = static_cast<float>(size.y),
    };

    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(ctx->Map(cbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return;
    }
    std::memcpy(mapped.pData, &data, sizeof(data));
    ctx->Unmap(cbuffer, 0);
    ctx->Draw(6, 0);
}

auto Backend::draw_rect(const Vec4I rect, const Color color, const int thickness) const -> void {
    draw_rect_fill({rect.x, rect.y, rect.z, thickness}, color);
    draw_rect_fill({rect.x, rect.y + rect.w - thickness, rect.z, thickness}, color);
    draw_rect_fill({rect.x, rect.y + thickness, thickness, rect.w - (thickness * 2)}, color);
    draw_rect_fill({rect.x + rect.z - thickness, rect.y + thickness, thickness, rect.w - (thickness * 2)}, color);
}

auto Backend::draw_line(const Vec2I from, const Vec2I to, const Color color, const int thickness) const -> void {
    const Vec2I d = to - from;
    if (std::abs(d.x) >= std::abs(d.y)) {
        draw_rect_fill({std::min(from.x, to.x), from.y - (thickness / 2), std::abs(d.x) + thickness, thickness}, color);
    } else {
        draw_rect_fill({from.x - (thickness / 2), std::min(from.y, to.y), thickness, std::abs(d.y) + thickness}, color);
    }
}

auto Backend::draw_circle_fill(const Vec2I center, const int radius, const Color color) const -> void {
    draw_rect_fill({center.x - radius, center.y - radius, radius * 2, radius * 2}, color);
}

auto Backend::draw_text(const std::string_view text, const Vec2I pos, const Color color, const float font_size) -> void {
    if (ctx == nullptr || text_cb == nullptr || font_srv == nullptr) {
        return;
    }

    const float scale = font_size / this->font_size;

    ctx->VSSetShader(text_vs, nullptr, 0);
    ctx->PSSetShader(text_ps, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &font_srv);
    ctx->PSSetSamplers(0, 1, &font_sampler);
    ctx->OMSetBlendState(bs_alpha, nullptr, 0xFFFFFFFF);

    TextCB cb{};
    cb.c_r = color.r() / 255.0F;
    cb.c_g = color.g() / 255.0F;
    cb.c_b = color.b() / 255.0F;
    cb.c_a = color.a() / 255.0F;
    cb.s_w = static_cast<float>(size.x);
    cb.s_h = static_cast<float>(size.y);

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
            bc = glyphs.data();
            char_idx = cp - FONT_FIRST;
        } else {
            const auto it = cjk_glyphs.find(cp);
            if (it != cjk_glyphs.end()) {
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

        const float total = scale * dpi_scale;
        cb.r_x = q.x0 * total;
        cb.r_y = q.y0 * total;
        cb.r_w = (q.x1 - q.x0) * total;
        cb.r_h = (q.y1 - q.y0) * total;
        cb.uv_u = q.s0;
        cb.uv_v = q.t0;
        cb.uv_w = q.s1 - q.s0;
        cb.uv_h = q.t1 - q.t0;

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(ctx->Map(text_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            break;
        }
        std::memcpy(mapped.pData, &cb, sizeof(cb));
        ctx->Unmap(text_cb, 0);

        ctx->VSSetConstantBuffers(0, 1, &text_cb);
        ctx->Draw(6, 0);
    }

    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->PSSetShaderResources(0, 0, nullptr);
    ctx->VSSetConstantBuffers(0, 1, &cbuffer);
    ctx->OMSetBlendState(bs_opaque, nullptr, 0xFFFFFFFF);
}

auto Backend::init_device(const HWND hwnd) -> bool {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    #ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    constexpr std::array levels{D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,};
    ID3D11Device* device_raw{};
    ID3D11DeviceContext* ctx_raw{};
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels.data(), levels.size(), D3D11_SDK_VERSION, &device_raw, nullptr, &ctx_raw);
    if (FAILED(hr)) {
        #ifdef _DEBUG
        if ((hr == DXGI_ERROR_SDK_COMPONENT_MISSING) || (hr == E_INVALIDARG)) {
            std::println(stderr, "[NekoUI] DX11 debug layer not available, retrying without debug flag");
            flags &= ~D3D11_CREATE_DEVICE_DEBUG;
            hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels.data(), levels.size(), D3D11_SDK_VERSION, &device_raw, nullptr, &ctx_raw);
        }
        #endif
        if (FAILED(hr)) {
            std::println(stderr, "[NekoUI] DX11 device init failed");
            return false;
        }
    }
    device_raw->QueryInterface(&device);
    ctx_raw->QueryInterface(&ctx);
    device_raw->Release();
    ctx_raw->Release();

    IDXGIFactory2* factory{};
    {
        IDXGIDevice1* dxgi_device{};
        IDXGIAdapter* adapter{};
        if (SUCCEEDED(device->QueryInterface(&dxgi_device)) && SUCCEEDED(dxgi_device->GetAdapter(&adapter))) {
            adapter->GetParent(IID_PPV_ARGS(&factory));
            adapter->Release();
            dxgi_device->Release();
        }
    }
    if (factory == nullptr) {
        std::println(stderr, "[NekoUI] DXGI factory init failed");
        return false;
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

    if (FAILED(factory->CreateSwapChainForHwnd(device, hwnd, &sc_desc, nullptr, nullptr, &swap_chain))) {
        factory->Release();
        std::println(stderr, "[NekoUI] SwapChain init failed");
        return false;
    }
    factory->Release();

    size = {static_cast<int>(sc_desc.Width), static_cast<int>(sc_desc.Height)};
    ID3D11Texture2D* back_buffer{};
    if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
        back_buffer->Release();
    }

    const UINT dpi = GetDpiForWindow(hwnd);
    dpi_scale = dpi > 0 ? static_cast<float>(dpi) / 96.0F : 1.0F;
    return true;
}

auto Backend::init_shaders() -> bool {
    ID3DBlob* vs_blob{};
    ID3DBlob* ps_blob{};
    if (!compile_shader(shader_src, "vs_main", "vs_5_0", &vs_blob)) {
        return false;
    }
    if (!compile_shader(shader_src, "ps_main", "ps_5_0", &ps_blob)) {
        vs_blob->Release();
        return false;
    }
    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vs);
    device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &ps);
    device->CreateInputLayout(nullptr, 0, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &layout);
    vs_blob->Release();
    ps_blob->Release();

    D3D11_BUFFER_DESC cb_desc{};
    cb_desc.ByteWidth = sizeof(float) * 12;
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cb_desc, nullptr, &cbuffer);

    ID3DBlob* tvs_blob{};
    ID3DBlob* tps_blob{};
    if (compile_shader(text_shader_src, "vs_main", "vs_5_0", &tvs_blob) && compile_shader(text_shader_src, "ps_main", "ps_5_0", &tps_blob)) {
        device->CreateVertexShader(tvs_blob->GetBufferPointer(), tvs_blob->GetBufferSize(), nullptr, &text_vs);
        device->CreatePixelShader(tps_blob->GetBufferPointer(), tps_blob->GetBufferSize(), nullptr, &text_ps);
        tvs_blob->Release();
        tps_blob->Release();

        D3D11_BUFFER_DESC tcb_desc{};
        tcb_desc.ByteWidth = sizeof(TextCB);
        tcb_desc.Usage = D3D11_USAGE_DYNAMIC;
        tcb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        tcb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&tcb_desc, nullptr, &text_cb);
    }
    return true;
}

auto Backend::init_states() -> bool {
    D3D11_RASTERIZER_DESC rs_desc;
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = D3D11_CULL_NONE;
    rs_desc.DepthClipEnable = 1;
    device->CreateRasterizerState(&rs_desc, &rs);

    D3D11_BLEND_DESC bd;
    bd.IndependentBlendEnable = 0;
    auto& [BlendEnable, SrcBlend, DestBlend, BlendOp, SrcBlendAlpha, DestBlendAlpha, BlendOpAlpha, RenderTargetWriteMask] = bd.RenderTarget[0];
    RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    BlendEnable = 0;
    device->CreateBlendState(&bd, &bs_opaque);

    BlendEnable = 1;
    SrcBlend = D3D11_BLEND_SRC_ALPHA;
    DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    BlendOp = D3D11_BLEND_OP_ADD;
    SrcBlendAlpha = D3D11_BLEND_ONE;
    DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendOpAlpha = D3D11_BLEND_OP_ADD;
    device->CreateBlendState(&bd, &bs_alpha);
    return true;
}

auto Backend::init_font() -> bool {
    if (text_cb == nullptr) {
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

    font_size = 16.0F;

    std::vector<unsigned char> bitmap(static_cast<size_t>(ATLAS_W) * ATLAS_H, 0);
    std::vector<stbtt_packedchar> cjk_output(CJK_LAST - CJK_FIRST + 1);

    stbtt_pack_context pc{};
    stbtt_PackBegin(&pc, bitmap.data(), ATLAS_W, ATLAS_H, 0, 1, nullptr);
    stbtt_PackSetOversampling(&pc, 2, 2);

    std::array<stbtt_pack_range, 2> ranges{};
    ranges[0].font_size = font_size;
    ranges[0].first_unicode_codepoint_in_range = FONT_FIRST;
    ranges[0].num_chars = FONT_COUNT;
    ranges[0].chardata_for_range = glyphs.data();

    ranges[1].font_size = font_size;
    ranges[1].first_unicode_codepoint_in_range = CJK_FIRST;
    ranges[1].num_chars = CJK_LAST - CJK_FIRST + 1;
    ranges[1].chardata_for_range = cjk_output.data();

    stbtt_PackFontRanges(&pc, font_data.data(), 0, ranges.data(), 2);
    stbtt_PackEnd(&pc);

    for (int i = 0; i < CJK_LAST - CJK_FIRST + 1; i++) {
        const auto& g = cjk_output[i];
        if (g.x0 < g.x1 && g.y0 < g.y1) {
            cjk_glyphs[CJK_FIRST + i] = g;
        }
    }

    std::println(stderr, "{} CJK glyphs baked", cjk_glyphs.size());

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
    if (SUCCEEDED(device->CreateTexture2D(&td, &sd, &tex))) {
        device->CreateShaderResourceView(tex, nullptr, &font_srv);
        tex->Release();
    }

    D3D11_SAMPLER_DESC sm;
    sm.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    sm.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    sm.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    device->CreateSamplerState(&sm, &font_sampler);
    return true;
}
