// 2026-08-10 12:53:39

#ifdef _WIN32
#include "Pipeline.hpp"

#include <cstdio>
#include <d3dcompiler.h>
#include <print>

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
    Pipeline::Pipeline(const Surface& surface) :
        surface_{surface} {
        init_shaders(surface);
        init_states(surface);
    }

    Pipeline::~Pipeline() {
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
    }

    auto Pipeline::bind_default() const -> void {
        ID3D11DeviceContext* ctx = surface_.context();
        if (ctx == nullptr) {
            return;
        }
        ctx->RSSetState(rs_);
        ctx->OMSetBlendState(bs_opaque_, nullptr, 0xFFFFFFFF);
        ctx->IASetInputLayout(layout_);
        ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        ctx->VSSetShader(vs_, nullptr, 0);
        ctx->PSSetShader(ps_, nullptr, 0);
        ctx->VSSetConstantBuffers(0, 1, &cbuffer_);
    }

    auto Pipeline::rect_cbuffer() const -> ID3D11Buffer* {
        return cbuffer_;
    }

    auto Pipeline::text_vs() const -> ID3D11VertexShader* {
        return text_vs_;
    }

    auto Pipeline::text_ps() const -> ID3D11PixelShader* {
        return text_ps_;
    }

    auto Pipeline::text_cbuffer() const -> ID3D11Buffer* {
        return text_cb_;
    }

    auto Pipeline::text_blend() const -> ID3D11BlendState* {
        return bs_text_;
    }

    auto Pipeline::opaque_blend() const -> ID3D11BlendState* {
        return bs_opaque_;
    }

    auto Pipeline::init_shaders(const Surface& surface) -> bool {
        ID3D11Device* device = surface.device();
        ID3DBlob* vs_blob{};
        ID3DBlob* ps_blob{};
        if (!compile_shader(shader_src, "vs_main", "vs_5_0", &vs_blob)) {
            return false;
        }
        if (!compile_shader(shader_src, "ps_main", "ps_5_0", &ps_blob)) {
            vs_blob->Release();
            return false;
        }
        device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &vs_);
        device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &ps_);
        device->CreateInputLayout(nullptr, 0, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &layout_);
        vs_blob->Release();
        ps_blob->Release();

        D3D11_BUFFER_DESC cb_desc{};
        cb_desc.ByteWidth = sizeof(float) * 12;
        cb_desc.Usage = D3D11_USAGE_DYNAMIC;
        cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        device->CreateBuffer(&cb_desc, nullptr, &cbuffer_);

        ID3DBlob* tvs_blob{};
        ID3DBlob* tps_blob{};
        if (compile_shader(text_shader_src, "vs_main", "vs_5_0", &tvs_blob) && compile_shader(text_shader_src, "ps_main", "ps_5_0", &tps_blob)) {
            device->CreateVertexShader(tvs_blob->GetBufferPointer(), tvs_blob->GetBufferSize(), nullptr, &text_vs_);
            device->CreatePixelShader(tps_blob->GetBufferPointer(), tps_blob->GetBufferSize(), nullptr, &text_ps_);
            tvs_blob->Release();
            tps_blob->Release();

            D3D11_BUFFER_DESC tcb_desc{};
            tcb_desc.ByteWidth = sizeof(TextCB);
            tcb_desc.Usage = D3D11_USAGE_DYNAMIC;
            tcb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
            tcb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            device->CreateBuffer(&tcb_desc, nullptr, &text_cb_);
        }
        return true;
    }

    auto Pipeline::init_states(const Surface& surface) -> bool {
        ID3D11Device* device = surface.device();
        D3D11_RASTERIZER_DESC rs_desc{};
        rs_desc.FillMode = D3D11_FILL_SOLID;
        rs_desc.CullMode = D3D11_CULL_NONE;
        rs_desc.DepthClipEnable = 1;
        device->CreateRasterizerState(&rs_desc, &rs_);

        D3D11_BLEND_DESC bd{};
        bd.IndependentBlendEnable = 0;
        auto& [BlendEnable, SrcBlend, DestBlend, BlendOp, SrcBlendAlpha, DestBlendAlpha, BlendOpAlpha, RenderTargetWriteMask] = bd.RenderTarget[0];
        RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        BlendEnable = 0;
        device->CreateBlendState(&bd, &bs_opaque_);

        // 文本专用预乘混合：着色器输出已预乘 alpha，避免深色背景暗边 fringing
        BlendEnable = 1;
        SrcBlend = D3D11_BLEND_ONE;
        DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        BlendOp = D3D11_BLEND_OP_ADD;
        SrcBlendAlpha = D3D11_BLEND_ONE;
        DestBlendAlpha = D3D11_BLEND_ZERO;
        BlendOpAlpha = D3D11_BLEND_OP_ADD;
        device->CreateBlendState(&bd, &bs_text_);
        return true;
    }
} // namespace neko::backend
#endif
