#pragma once
#ifdef _WIN32
#include <DirectXMath.h>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <array>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "../Backend.hpp"
#include "../stb_truetype.h"

namespace neko::backend {
    class DirectX11 final : public Backend {
    public:
        explicit DirectX11(HWND hwnd);
        ~DirectX11() override;

        auto resize(Vec2I new_size) -> void override;
        auto set_dpi(unsigned int dpi) -> void override;
        [[nodiscard]] auto get_dpi_scale() const -> float override;
        auto begin() const -> void override;
        auto end() const -> void override;
        auto draw_rect_fill(Vec4I rect, Color color) const -> void override;
        auto draw_rect(Vec4I rect, Color color, int thickness) const -> void override;
        auto draw_line(Vec2I from, Vec2I to, Color color, int thickness) const -> void override;
        auto draw_circle_fill(Vec2I center, int radius, Color color) const -> void override;
        auto draw_text(std::string_view text, Vec2I pos, Color color, float font_size = 16.0F) -> void override;

        [[nodiscard]] auto get_native_handle() const -> void* override {
            return hwnd_;
        }
    private:
        static constexpr int ATLAS_W = 4096;
        static constexpr int ATLAS_H = 4096;
        static constexpr int FONT_FIRST = 32;
        static constexpr int FONT_COUNT = 96;
        static constexpr int CJK_FIRST = 0x4E00;
        static constexpr int CJK_LAST = 0x9FFF;

        struct TextCB {
            float r_x, r_y, r_w, r_h;
            float c_r, c_g, c_b, c_a;
            float s_w, s_h;
            float uv_u, uv_v;
            float uv_w, uv_h;
            float p0, p1;
        };

        auto init_device() -> void;
        auto init_swap_chain(HWND hwnd) -> void;
        auto init_shaders() -> bool;
        auto init_states() -> bool;
        auto init_font() -> bool;

        ID3D11Device* device_{};
        ID3D11DeviceContext* ctx_{};
        IDXGISwapChain1* swap_chain_{};
        ID3D11RenderTargetView* rtv_{};
        ID3D11VertexShader* vs_{};
        ID3D11PixelShader* ps_{};
        ID3D11InputLayout* layout_{};
        ID3D11RasterizerState* rs_{};
        ID3D11BlendState* bs_alpha_{};
        ID3D11BlendState* bs_opaque_{};
        ID3D11Buffer* cbuffer_{};
        Vec2I size_{};

        ID3D11ShaderResourceView* font_srv_{};
        ID3D11SamplerState* font_sampler_{};
        ID3D11VertexShader* text_vs_{};
        ID3D11PixelShader* text_ps_{};
        ID3D11Buffer* text_cb_{};
        float font_size_{};
        float dpi_scale_ = 1.0F;
        HWND hwnd_{};
        std::array<stbtt_packedchar, FONT_COUNT> glyphs_{};
        std::unordered_map<int, stbtt_packedchar> cjk_glyphs_{};
    };
}
#endif
