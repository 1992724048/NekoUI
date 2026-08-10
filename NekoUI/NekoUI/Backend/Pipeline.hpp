// 2026-08-10 12:53:37

#pragma once
#ifdef _WIN32
#include <d3d11.h>

#include "Surface.hpp"

namespace neko::backend {
    class Pipeline final {
    public:
        explicit Pipeline(const Surface& surface);
        ~Pipeline();

        Pipeline(const Pipeline&) = delete;
        auto operator=(const Pipeline&) -> Pipeline& = delete;

        struct TextCB {
            float r_x, r_y, r_w, r_h;
            float c_r, c_g, c_b, c_a;
            float s_w, s_h;
            float uv_u, uv_v;
            float uv_w, uv_h;
            float p0, p1;
        };

        auto bind_default() const -> void;
        [[nodiscard]] auto rect_cbuffer() const -> ID3D11Buffer*;
        [[nodiscard]] auto text_vs() const -> ID3D11VertexShader*;
        [[nodiscard]] auto text_ps() const -> ID3D11PixelShader*;
        [[nodiscard]] auto text_cbuffer() const -> ID3D11Buffer*;
        [[nodiscard]] auto text_blend() const -> ID3D11BlendState*;
        [[nodiscard]] auto opaque_blend() const -> ID3D11BlendState*;
    private:
        const Surface& surface_;
        ID3D11VertexShader* vs_{};
        ID3D11PixelShader* ps_{};
        ID3D11InputLayout* layout_{};
        ID3D11RasterizerState* rs_{};
        ID3D11BlendState* bs_opaque_{};
        ID3D11BlendState* bs_text_{};
        ID3D11Buffer* cbuffer_{};
        ID3D11VertexShader* text_vs_{};
        ID3D11PixelShader* text_ps_{};
        ID3D11Buffer* text_cb_{};

        auto init_shaders(const Surface& surface) -> bool;
        auto init_states(const Surface& surface) -> bool;
    };
}
#endif
