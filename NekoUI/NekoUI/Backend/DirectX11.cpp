// 2026-08-10 10:18:55

#ifdef _WIN32
#include "DirectX11.hpp"

#include <algorithm>
#include <array>
#include <print>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

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
        release_font_resources();
        if (font_sampler_ != nullptr) {
            font_sampler_->Release();
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

    auto DirectX11::get_client_size() const -> Vec2I {
        RECT client{};
        if (GetClientRect(hwnd_, &client) != 0) {
            return {.x = client.right - client.left, .y = client.bottom - client.top};
        }
        return {};
    }

    auto DirectX11::init_device() -> void {
        UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        #ifdef _DEBUG
        create_flags |= D3D11_CREATE_DEVICE_DEBUG;
        #endif
        D3D_FEATURE_LEVEL feature_level{};
        if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags, nullptr, 0, D3D11_SDK_VERSION, &device_, &feature_level, &ctx_))) {
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

        size_ = {.x = static_cast<int>(sc_desc.Width), .y = static_cast<int>(sc_desc.Height)};
        ID3D11Texture2D* back_buffer{};
        if (SUCCEEDED(swap_chain_->GetBuffer(0, IID_PPV_ARGS(&back_buffer)))) {
            device_->CreateRenderTargetView(back_buffer, nullptr, &rtv_);
            back_buffer->Release();
        }

        const UINT dpi = GetDpiForWindow(hwnd);
        dpi_scale_ = dpi > 0 ? static_cast<float>(dpi) / 96.0F : 1.0F;
    }

} // namespace neko::backend
#endif
