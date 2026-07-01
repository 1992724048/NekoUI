#include "Backend.hpp"

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

    // DXGI Factory
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

    // SwapChain
    RECT rc{};
    GetClientRect(hwnd, &rc);

    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.Width = static_cast<UINT>(rc.right > 0 ? rc.right : 1);
    desc.Height = static_cast<UINT>(rc.bottom > 0 ? rc.bottom : 1);
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount = 2;
    desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

    if (FAILED(factory->CreateSwapChainForHwnd(device, hwnd, &desc, nullptr, nullptr, &swap_chain))) {
        factory->Release();
        OutputDebugStringA("[NekoUI] SwapChain init failed\n");
        return;
    }
    factory->Release();

    size = {static_cast<int>(desc.Width), static_cast<int>(desc.Height)};

    // RenderTargetView
    ID3D11Texture2D* back_buffer{};
    if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
        device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
        back_buffer->Release();
    }

    // 编译着色器
    auto compile = [](const char* entry, const char* target, ID3DBlob** blob) -> bool {
        ID3DBlob* error{};
        const HRESULT hr = D3DCompile(shader_src, strlen(shader_src), nullptr, nullptr, nullptr, entry, target, D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, blob, &error);
        if (FAILED(hr)) {
            if (error) {
                OutputDebugStringA(static_cast<const char*>(error->GetBufferPointer()));
                error->Release();
            }
            return false;
        }
        return true;
    };

    ID3DBlob* vs_blob{};
    ID3DBlob* ps_blob{};
    if (!compile("vs_main", "vs_5_0", &vs_blob)) {
        return;
    }
    if (!compile("ps_main", "ps_5_0", &ps_blob)) {
        vs_blob->Release();
        return;
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
    device->CreateBuffer(&cb_desc, nullptr, &cb_rect);
}

Backend::~Backend() {
    if (cb_rect != nullptr) {
        cb_rect->Release();
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
        ID3D11Texture2D* back_buffer{};
        if (SUCCEEDED(swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
            device->CreateRenderTargetView(back_buffer, nullptr, &rtv);
            back_buffer->Release();
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

    constexpr std::array<float, 4> clear{0.11F, 0.11F, 0.13F, 1.0F};
    ctx->ClearRenderTargetView(rtv, clear.data());

    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->PSSetShader(ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cb_rect);
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
    };
    const RectData data{
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
    if (FAILED(ctx->Map(cb_rect, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
        return;
    }
    std::memcpy(mapped.pData, &data, sizeof(data));
    ctx->Unmap(cb_rect, 0);
    ctx->Draw(6, 0);
}

auto Backend::draw_rect(glm::ivec4 rect, const Color color, int thickness) const -> void {
    draw_rect_fill({rect.x, rect.y, rect.z, thickness}, color);
    draw_rect_fill({rect.x, rect.y + rect.w - thickness, rect.z, thickness}, color);
    draw_rect_fill({rect.x, rect.y + thickness, thickness, rect.w - thickness * 2}, color);
    draw_rect_fill({rect.x + rect.z - thickness, rect.y + thickness, thickness, rect.w - thickness * 2}, color);
}

auto Backend::draw_line(const glm::ivec2 from, const glm::ivec2 to, const Color color, int thickness) const -> void {
    const glm::ivec2 d = to - from;
    if (std::abs(d.x) >= std::abs(d.y)) {
        const float len = static_cast<float>(std::abs(d.x)) + thickness;
        draw_rect_fill({std::min(from.x, to.x), from.y - thickness / 2, static_cast<int>(len), thickness}, color);
    } else {
        const float len = static_cast<float>(std::abs(d.y)) + thickness;
        draw_rect_fill({from.x - thickness / 2, std::min(from.y, to.y), thickness, static_cast<int>(len)}, color);
    }
}

auto Backend::draw_circle_fill(const glm::ivec2 center, const int radius, const Color color) const -> void {
    draw_rect_fill({center.x - radius, center.y - radius, radius * 2, radius * 2}, color);
}

auto Backend::draw_text(std::string_view text, glm::ivec2 pos, Color color, float font_size) -> void {
    // TODO: DirectWrite
}
