// 2026-07-02 01:14:53

#include "Backend.hpp"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include <array>
#include <cstdint>
#include <d3dcommon.h>
#include <d3dcompiler.h>

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
    uint2 uv = uint2(vid & 1, (vid >> 1) & 1);
    float2 pos = rect.xy + float2(uv) * rect.zw;
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
    uint2 uv = uint2(vid & 1, (vid >> 1) & 1);
    float2 pos = rect.xy + float2(uv) * rect.zw;
    float2 clip = pos / screen_size * 2.0 - 1.0;
    clip.y = -clip.y;

    VSOutput o;
    o.pos = float4(clip, 0, 1);
    o.col = color;
    o.uv = uv_offset + float2(uv) * uv_size;
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
                OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
                error->Release();
            }
            return false;
        }
        return true;
    }
} // namespace

Backend::Backend(const HWND hwnd) {
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    #ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    constexpr std::array levels{D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,};

    ID3D11Device* device_raw{};
    ID3D11DeviceContext* ctx_raw{};
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels.data(), levels.size(), D3D11_SDK_VERSION, &device_raw, nullptr, &ctx_raw))) {
        OutputDebugStringA("[NekoUI] DX11 device init failed\n");
        return;
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
        OutputDebugStringA("[NekoUI] DXGI factory init failed\n");
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

    if (FAILED(factory->CreateSwapChainForHwnd(device, hwnd, &sc_desc, nullptr, nullptr, &swap_chain))) {
        factory->Release();
        OutputDebugStringA("[NekoUI] SwapChain init failed\n");
        return;
    }
    factory->Release();

    size = {static_cast<int>(sc_desc.Width), static_cast<int>(sc_desc.Height)};

    ID3D11Texture2D* back_buffer{};
    if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
        back_buffer->Release();
    }

    ID3DBlob* vs_blob{};
    ID3DBlob* ps_blob{};
    if (!compile_shader(shader_src, "vs_main", "vs_5_0", &vs_blob)) {
        return;
    }
    if (!compile_shader(shader_src, "ps_main", "ps_5_0", &ps_blob)) {
        vs_blob->Release();
        return;
    }

    device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vs);
    device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &ps);
    device->CreateInputLayout(nullptr, 0, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &layout);
    vs_blob->Release();
    ps_blob->Release();

    D3D11_BUFFER_DESC cb_desc{};
    cb_desc.ByteWidth = sizeof(float) * 12; // 48 bytes
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    device->CreateBuffer(&cb_desc, nullptr, &cbuffer);

    D3D11_RASTERIZER_DESC rs_desc{};
    rs_desc.FillMode = D3D11_FILL_SOLID;
    rs_desc.CullMode = D3D11_CULL_NONE;
    rs_desc.DepthClipEnable = 1;
    device->CreateRasterizerState(&rs_desc, &rs);

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

    if (text_cb != nullptr) {
        FILE* fp = nullptr;
        errno_t err = _wfopen_s(&fp, L"C:\\Windows\\Fonts\\segoeui.ttf", L"rb");
        if (err == 0 && (fp != nullptr)) {
            fseek(fp, 0, SEEK_END);
            const long len = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            auto* font_data = static_cast<unsigned char*>(malloc(len));
            if ((font_data != nullptr) && fread(font_data, 1, len, fp) == static_cast<size_t>(len)) {
                // Bake bitmap
                font_size = 14.0F;
                auto* bitmap = static_cast<unsigned char*>(calloc(ATLAS_W * ATLAS_H, 1));
                stbtt_BakeFontBitmap(font_data, 0, font_size, bitmap, ATLAS_W, ATLAS_H, FONT_FIRST, FONT_COUNT, glyphs);

                // 上传为 A8 纹理
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
                sd.pSysMem = bitmap;
                sd.SysMemPitch = ATLAS_W;

                if (SUCCEEDED(device->CreateTexture2D(&td, &sd, &tex))) {
                    device->CreateShaderResourceView(tex, nullptr, &font_srv);
                    tex->Release();
                }
                free(bitmap);
            }
            free(font_data);
            fclose(fp);
        }

        // 采样器
        D3D11_SAMPLER_DESC sm{};
        sm.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
        sm.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        sm.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        device->CreateSamplerState(&sm, &font_sampler);
    }
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

auto Backend::resize(const glm::ivec2 new_size) -> void {
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

auto Backend::begin() const -> void {
    ctx->OMSetRenderTargets(1, &rtv, nullptr);
    D3D11_VIEWPORT vp{};
    vp.Width = static_cast<float>(size.x);
    vp.Height = static_cast<float>(size.y);
    vp.MaxDepth = 1.0F;
    ctx->RSSetViewports(1, &vp);

    constexpr std::array clear{0.11F, 0.11F, 0.13F, 1.0F};
    ctx->ClearRenderTargetView(rtv, clear.data());

    ctx->RSSetState(rs);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cbuffer);
}

auto Backend::end() const -> void {
    if (render_callback) {
        render_callback();
    }
    swap_chain->Present(1, 0);
}

auto Backend::draw_rect_fill(const glm::ivec4 rect, const Color color) const -> void {
    struct RectData {
        float r_x, r_y, r_w, r_h;
        float c_r, c_g, c_b, c_a;
        float s_w, s_h;
        float _p0, _p1;
    } const data{
        .r_x = static_cast<float>(rect.x),
        .r_y = static_cast<float>(rect.y),
        .r_w = static_cast<float>(rect.z),
        .r_h = static_cast<float>(rect.w),
        .c_r = color.r / 255.0F,
        .c_g = color.g / 255.0F,
        .c_b = color.b / 255.0F,
        .c_a = color.a / 255.0F,
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

auto Backend::draw_rect(const glm::ivec4 rect, const Color color, const int thickness) const -> void {
    draw_rect_fill({rect.x, rect.y, rect.z, thickness}, color);
    draw_rect_fill({rect.x, rect.y + rect.w - thickness, rect.z, thickness}, color);
    draw_rect_fill({rect.x, rect.y + thickness, thickness, rect.w - (thickness * 2)}, color);
    draw_rect_fill({rect.x + rect.z - thickness, rect.y + thickness, thickness, rect.w - (thickness * 2)}, color);
}

auto Backend::draw_line(const glm::ivec2 from, const glm::ivec2 to, const Color color, const int thickness) const -> void {
    const glm::ivec2 d = to - from;
    if (std::abs(d.x) >= std::abs(d.y)) {
        draw_rect_fill({std::min(from.x, to.x), from.y - (thickness / 2), std::abs(d.x) + thickness, thickness}, color);
    } else {
        draw_rect_fill({from.x - (thickness / 2), std::min(from.y, to.y), thickness, std::abs(d.y) + thickness}, color);
    }
}

auto Backend::draw_circle_fill(const glm::ivec2 center, const int radius, const Color color) const -> void {
    draw_rect_fill({center.x - radius, center.y - radius, radius * 2, radius * 2}, color);
}

auto Backend::draw_text(const std::string_view text, const glm::ivec2 pos, const Color color, float /*font_size*/) const -> void {
    if (text.empty() || text_cb == nullptr || font_srv == nullptr) {
        return;
    }

    ctx->VSSetShader(text_vs, nullptr, 0);
    ctx->PSSetShader(text_ps, nullptr, 0);
    ctx->PSSetShaderResources(0, 1, &font_srv);
    ctx->PSSetSamplers(0, 1, &font_sampler);

    auto x = static_cast<float>(pos.x);
    auto y = static_cast<float>(pos.y);

    TextCB cb{};
    cb.c_r = color.r / 255.0F;
    cb.c_g = color.g / 255.0F;
    cb.c_b = color.b / 255.0F;
    cb.c_a = color.a / 255.0F;
    cb.s_w = static_cast<float>(size.x);
    cb.s_h = static_cast<float>(size.y);

    for (const auto ch : text) {
        const int c = static_cast<unsigned char>(ch);
        if (c < FONT_FIRST || c >= FONT_FIRST + FONT_COUNT) {
            continue;
        }

        stbtt_aligned_quad q{};
        stbtt_GetBakedQuad(glyphs, ATLAS_W, ATLAS_H, c - FONT_FIRST, &x, &y, &q, 1);

        cb.r_x = q.x0;
        cb.r_y = q.y0;
        cb.r_w = q.x1 - q.x0;
        cb.r_h = q.y1 - q.y0;
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
}
