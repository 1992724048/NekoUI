// 2026-08-10 12:53:44

#ifdef _WIN32
#include "Surface.hpp"

#include <algorithm>
#include <array>
#include <print>

namespace neko::backend {
    Surface::Surface(const HWND hwnd) :
        hwnd_{hwnd} {
        init_device();
        init_swap_chain(hwnd);
    }

    Surface::~Surface() {
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

    auto Surface::resize(const Vec2I new_size) -> void {
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

    auto Surface::set_dpi(const unsigned int dpi) -> void {
        dpi_scale_ = static_cast<float>(dpi) / 96.0F;
    }

    auto Surface::get_dpi_scale() const -> float {
        return dpi_scale_;
    }

    auto Surface::begin_rt() const -> void {
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
    }

    auto Surface::end() const -> void {
        if (swap_chain_ == nullptr) {
            return;
        }
        swap_chain_->Present(1, 0);
    }

    auto Surface::device() const -> ID3D11Device* {
        return device_;
    }

    auto Surface::context() const -> ID3D11DeviceContext* {
        return ctx_;
    }

    auto Surface::size() const -> Vec2I {
        return size_;
    }

    auto Surface::native_handle() const -> Handle {
        return hwnd_;
    }

    auto Surface::client_size() const -> Vec2I {
        RECT client{};
        if (GetClientRect(hwnd_, &client) != 0) {
            return {.x = client.right - client.left, .y = client.bottom - client.top};
        }
        return {};
    }

    auto Surface::init_device() -> void {
        UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        #ifdef _DEBUG
        create_flags |= D3D11_CREATE_DEVICE_DEBUG;
        #endif
        D3D_FEATURE_LEVEL feature_level{};
        const HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_flags, nullptr, 0, D3D11_SDK_VERSION, &device_, &feature_level, &ctx_);
        std::println(stderr, "[diag] D3D11CreateDevice hr={:#010X} feat={} dev={} ctx={}", static_cast<unsigned int>(hr), static_cast<int>(feature_level), device_ != nullptr, ctx_ != nullptr);
        if (FAILED(hr)) {
            std::println(stderr, "[NekoUI] D3D11CreateDevice failed");
        }
    }

    auto Surface::init_swap_chain(const HWND hwnd) -> void {
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
