#pragma once
#include <DirectXMath.h>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <functional>
#include <string>
#include <string_view>

#include "stb_truetype.h"

#include "../Type.hpp"

namespace neko::backend {
    using namespace neko::type;

    //! @brief 单窗口 DirectX 11 渲染后端
    class Backend final {
    public:
        explicit Backend(HWND hwnd);
        ~Backend();

        Backend(const Backend&) = delete;
        auto operator=(const Backend&) -> Backend& = delete;

        auto resize(glm::ivec2 new_size) -> void;

        auto begin() const -> void;
        auto end() const -> void;

        auto draw_rect_fill(glm::ivec4 rect, Color color) const -> void;
        auto draw_rect(glm::ivec4 rect, Color color, int thickness) const -> void;
        auto draw_line(glm::ivec2 from, glm::ivec2 to, Color color, int thickness) const -> void;
        auto draw_circle_fill(glm::ivec2 center, int radius, Color color) const -> void;
        auto draw_text(std::string_view text, glm::ivec2 pos, Color color, float font_size) const -> void;

        std::function<void()> render_callback;
    private:
        static constexpr int ATLAS_W = 1024;
        static constexpr int ATLAS_H = 1024;
        static constexpr int FONT_FIRST = 32;
        static constexpr int FONT_COUNT = 96; // 32..127

        struct TextCB {
            float r_x, r_y, r_w, r_h;
            float c_r, c_g, c_b, c_a;
            float s_w, s_h;
            float uv_u, uv_v;
            float uv_w, uv_h;
            float _p0, _p1;
        };

        ID3D11Device* device{};
        ID3D11DeviceContext* ctx{};
        IDXGISwapChain1* swap_chain{};
        ID3D11RenderTargetView* rtv{};
        ID3D11VertexShader* vs{};
        ID3D11PixelShader* ps{};
        ID3D11InputLayout* layout{};
        ID3D11RasterizerState* rs{};
        ID3D11Buffer* cbuffer{};
        glm::ivec2 size{};

        // 字体图集
        ID3D11ShaderResourceView* font_srv{};
        ID3D11SamplerState* font_sampler{};
        ID3D11VertexShader* text_vs{};
        ID3D11PixelShader* text_ps{};
        ID3D11Buffer* text_cb{};
        float font_size{};
        stbtt_bakedchar glyphs[FONT_COUNT]{};
    };
} // namespace neko::backend
