#pragma once
#include "../Backend.hpp"

#include <DirectXMath.h>
#include <Windows.h>
#include <d3d11.h>
#include <d3d11_4.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <wrl/client.h>

namespace neko::backend::impl {
    class DirectX11 final : public Backend {
    public:
        static auto create(const std::shared_ptr<window::Window>& window) -> std::shared_ptr<DirectX11>;
        auto resize(Vec2<int> new_size) -> void override;
        auto submit() -> void override;
        ~DirectX11() noexcept override;
        auto draw_rect(Vec4<int> range, Color rgba, int thickness) -> void override;
    private:
        UINT msaa_quality{0};
        D3D11_VIEWPORT screen_viewport{};
        D3D_FEATURE_LEVEL feature_level{D3D_FEATURE_LEVEL_11_1};

        template<class T>
        using ComPtr = Microsoft::WRL::ComPtr<T>;

        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<IDXGISwapChain> swap_chain;

        ComPtr<ID3D11Device1> device1;
        ComPtr<ID3D11DeviceContext1> context1;
        ComPtr<IDXGISwapChain1> swap_chain1;

        ComPtr<IDXGIDevice> dxgi_device;
        ComPtr<IDXGIAdapter> dxgi_adapter;
        ComPtr<IDXGIFactory1> dxgi_factory1;
        ComPtr<IDXGIFactory2> dxgi_factory2;

        ComPtr<ID3D11Texture2D> depth_stencil_buffer;
        ComPtr<ID3D11RenderTargetView> render_target_view;
        ComPtr<ID3D11DepthStencilView> depth_stencil_view;
    };
} // namespace neko::backend::impl
