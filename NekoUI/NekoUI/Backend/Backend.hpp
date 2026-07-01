#pragma once
#include <DirectXMath.h>
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include <functional>
#include <string_view>

#include "../Type.hpp"

namespace neko::backend {
    using namespace neko::type;

    //! @brief 单窗口 DirectX 11 渲染后端
    class Backend final {
    public:
        //! @param hwnd 窗口句柄（立即创建设备 + SwapChain）
        explicit Backend(HWND hwnd);
        ~Backend();

        Backend(const Backend&) = delete;
        auto operator=(const Backend&) -> Backend& = delete;

        //! @brief 窗口尺寸变化时重建 SwapChain
        auto resize(glm::ivec2 new_size) -> void;

        //! @brief 开始一帧
        auto begin() const -> void;
        //! @brief 结束一帧并 Present
        auto end() const -> void;

        auto draw_rect_fill(glm::ivec4 rect, Color color) const -> void;
        auto draw_rect(glm::ivec4 rect, Color color, int thickness) const -> void;
        auto draw_line(glm::ivec2 from, glm::ivec2 to, Color color, int thickness) const -> void;
        auto draw_circle_fill(glm::ivec2 center, int radius, Color color) const -> void;
        auto draw_text(std::string_view text, glm::ivec2 pos, Color color, float font_size) -> void;

        std::function<void()> render_callback;
    private:
        ID3D11Device* device{};
        ID3D11DeviceContext* ctx{};
        IDXGISwapChain1* swap_chain{};
        ID3D11RenderTargetView* rtv{};
        ID3D11VertexShader* vs{};
        ID3D11PixelShader* ps{};
        ID3D11InputLayout* layout{};
        ID3D11Buffer* cb_rect{};
        glm::ivec2 size{};
    };
} // namespace neko::backend
