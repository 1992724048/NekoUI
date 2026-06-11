// 2026-06-03 04:54:57

#include "DirectX11.hpp"

#include <array>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "D3DCompiler.lib")

using namespace neko::backend::impl;

DirectX11::DirectX11(const std::shared_ptr<window::Window>& window) {
    HRESULT result{S_OK};

    if (!create_device()) {
        return;
    }

    if (const auto opt = create_swap_chain(window)) {
        this->windows[window->get_handle()] = opt.value();
    }
}

auto DirectX11::resize(const Handle window_handle, const Vec2<int> new_size) -> void {
    if (device == nullptr || !this->windows.contains(window_handle)) {
        return;
    }

    auto& child = this->windows[window_handle];

    child.render_target_view.Reset();
    child.depth_stencil_view.Reset();
    child.depth_stencil_buffer.Reset();

    HRESULT result{S_OK};
    ComPtr<ID3D11Texture2D> back_buffer;

    result = child.swap_chain->ResizeBuffers(dxgi_factory2 != nullptr ? 3 : 1,
                                             static_cast<UINT>(new_size.x),
                                             static_cast<UINT>(new_size.y),
                                             DXGI_FORMAT_R8G8B8A8_UNORM,
                                             dxgi_factory2 != nullptr ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0);
    assert(SUCCEEDED(result));
    result = child.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    assert(SUCCEEDED(result));
    result = device->CreateRenderTargetView(back_buffer.Get(), nullptr, child.render_target_view.GetAddressOf());
    assert(SUCCEEDED(result));

    back_buffer.Reset();

    D3D11_TEXTURE2D_DESC depth_stencil_desc;

    depth_stencil_desc.Width = new_size.x;
    depth_stencil_desc.Height = new_size.y;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.SampleDesc.Quality = 0;
    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_stencil_desc.CPUAccessFlags = 0;
    depth_stencil_desc.MiscFlags = 0;

    result = device->CreateTexture2D(&depth_stencil_desc, nullptr, child.depth_stencil_buffer.GetAddressOf());
    assert(SUCCEEDED(result));
    result = device->CreateDepthStencilView(child.depth_stencil_buffer.Get(), nullptr, child.depth_stencil_view.GetAddressOf());
    assert(SUCCEEDED(result));

    context->OMSetRenderTargets(1, child.render_target_view.GetAddressOf(), child.depth_stencil_view.Get());

    screen_viewport.TopLeftX = 0;
    screen_viewport.TopLeftY = 0;
    screen_viewport.Width = static_cast<float>(new_size.x);
    screen_viewport.Height = static_cast<float>(new_size.y);
    screen_viewport.MinDepth = 0.F;
    screen_viewport.MaxDepth = 1.F;

    context->RSSetViewports(1, &screen_viewport);
}

auto DirectX11::attach(const std::shared_ptr<window::Window>& window) -> bool {
    if (!create_swap_chain(window)) {
        return false;
    }

    if (const auto opt = create_swap_chain(window)) {
        this->windows[window->get_handle()] = opt.value();
    }
    return true;
}

auto DirectX11::submit(Handle window_handle) -> void {
    if (device == nullptr || !this->windows.contains(window_handle)) {
        return;
    }

    auto& child = this->windows[window_handle];
    if (context.Get() == nullptr) {
        return;
    }

    context->OMSetRenderTargets(1, child.render_target_view.GetAddressOf(), child.depth_stencil_view.Get());
    context->RSSetViewports(1, &screen_viewport);

    constexpr std::array color = {0.0F, 0.0F, 0.0F, 1.0F};
    context->ClearRenderTargetView(child.render_target_view.Get(), color.data());
    context->ClearDepthStencilView(child.depth_stencil_view.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.F, 0);

    render_callback();

    child.swap_chain->Present(1, 0);
}

auto DirectX11::get_handle() -> Handle {
    return device.GetAddressOf();
}

DirectX11::~DirectX11() noexcept {
    if (context.Get() != nullptr) {
        context->ClearState();
    }
}

auto DirectX11::draw_rect(Handle window_handle, Vec4<int> range, Color rgba, int thickness) -> void {
    std::array color = {rgba.r / 255.F, rgba.g / 255.F, rgba.b / 255.F, 1.0F};
    context->ClearRenderTargetView(render_target_view.Get(), color.data());
}

auto DirectX11::create_device() -> bool {
    constexpr std::array driver_types{D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE};
    constexpr std::array feature_levels{D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0};

    HRESULT result{S_OK};
    UINT create_device_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    #ifdef _DEBUG
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif

    for (const auto& driver_type : driver_types) {
        result = D3D11CreateDevice(nullptr,
                                   driver_type,
                                   nullptr,
                                   create_device_flags,
                                   feature_levels.data(),
                                   feature_levels.size(),
                                   D3D11_SDK_VERSION,
                                   this->device.GetAddressOf(),
                                   &this->feature_level,
                                   this->context.GetAddressOf());

        if (SUCCEEDED(result)) {
            break;
        }
    }

    if (FAILED(result)) {
        return false;
    }

    if (this->feature_level != D3D_FEATURE_LEVEL_11_0 && this->feature_level != D3D_FEATURE_LEVEL_11_1) {
        return false;
    }

    result = this->device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &this->msaa_quality);
    assert(SUCCEEDED(result));

    result = this->device.As(&this->dxgi_device);
    assert(SUCCEEDED(result));
    result = this->dxgi_device->GetAdapter(this->dxgi_adapter.GetAddressOf());
    assert(SUCCEEDED(result));
    result = this->dxgi_adapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(this->dxgi_factory1.GetAddressOf()));
    assert(SUCCEEDED(result));
    result = this->dxgi_factory1.As(&this->dxgi_factory2);
    assert(SUCCEEDED(result));
    return true;
}

auto DirectX11::create_swap_chain(const std::shared_ptr<window::Window>& window) -> ChildWindow {
    HRESULT result{S_OK};

    ChildWindow child{};

    const auto size = window->get_size();
    auto [client_width, client_height] = size;

    if (this->feature_level == D3D_FEATURE_LEVEL_11_1) {
        result = this->device.As(&this->device1);
        assert(SUCCEEDED(result));
        result = context.As(&context1);
        assert(SUCCEEDED(result));

        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc1{};
        swap_chain_desc1.Width = client_width;
        swap_chain_desc1.Height = client_height;
        swap_chain_desc1.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc1.SampleDesc.Count = 1;
        swap_chain_desc1.SampleDesc.Quality = 0;
        swap_chain_desc1.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc1.BufferCount = 3;
        swap_chain_desc1.Scaling = DXGI_SCALING_STRETCH;
        swap_chain_desc1.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc1.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_desc1.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fullscreen_desc;
        fullscreen_desc.RefreshRate.Numerator = 120;
        fullscreen_desc.RefreshRate.Denominator = 1;
        fullscreen_desc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        fullscreen_desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        fullscreen_desc.Windowed = TRUE;

        result = this->dxgi_factory2->CreateSwapChainForHwnd(this->device.Get(),
                                                             static_cast<HWND>(window->get_handle()),
                                                             &swap_chain_desc1,
                                                             &fullscreen_desc,
                                                             nullptr,
                                                             child.swap_chain1.GetAddressOf());
        assert(SUCCEEDED(result));
        result = child.swap_chain1.As(&child.swap_chain);
        assert(SUCCEEDED(result));
    } else {
        DXGI_SWAP_CHAIN_DESC swap_chain_desc;
        ZeroMemory(&swap_chain_desc, sizeof(swap_chain_desc));
        swap_chain_desc.BufferDesc.Width = client_width;
        swap_chain_desc.BufferDesc.Height = client_height;
        swap_chain_desc.BufferDesc.RefreshRate.Numerator = 120;
        swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
        swap_chain_desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        swap_chain_desc.SampleDesc.Count = 1;
        swap_chain_desc.SampleDesc.Quality = 0;
        swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount = 2;
        swap_chain_desc.OutputWindow = static_cast<HWND>(window->get_handle());
        swap_chain_desc.Windowed = TRUE;
        swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        swap_chain_desc.Flags = 0;
        result = this->dxgi_factory1->CreateSwapChain(this->device.Get(), &swap_chain_desc, child.swap_chain.GetAddressOf());
        assert(SUCCEEDED(result));
    }

    result = this->dxgi_factory1->MakeWindowAssociation(static_cast<HWND>(window->get_handle()), DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
    assert(SUCCEEDED(result));

    ComPtr<ID3D11Texture2D> back_buffer;
    result = child.swap_chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(back_buffer.GetAddressOf()));
    assert(SUCCEEDED(result));
    result = this->device->CreateRenderTargetView(back_buffer.Get(), nullptr, child.render_target_view.GetAddressOf());
    assert(SUCCEEDED(result));

    D3D11_TEXTURE2D_DESC depth_stencil_desc;

    depth_stencil_desc.Width = client_width;
    depth_stencil_desc.Height = client_height;
    depth_stencil_desc.MipLevels = 1;
    depth_stencil_desc.ArraySize = 1;
    depth_stencil_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    depth_stencil_desc.SampleDesc.Count = 1;
    depth_stencil_desc.SampleDesc.Quality = 0;

    depth_stencil_desc.Usage = D3D11_USAGE_DEFAULT;
    depth_stencil_desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    depth_stencil_desc.CPUAccessFlags = 0;
    depth_stencil_desc.MiscFlags = 0;

    result = this->device->CreateTexture2D(&depth_stencil_desc, nullptr, child.depth_stencil_buffer.GetAddressOf());
    assert(SUCCEEDED(result));
    result = this->device->CreateDepthStencilView(child.depth_stencil_buffer.Get(), nullptr, child.depth_stencil_view.GetAddressOf());
    assert(SUCCEEDED(result));
    context->OMSetRenderTargets(1, child.render_target_view.GetAddressOf(), child.depth_stencil_view.Get());

    this->screen_viewport.TopLeftX = 0;
    this->screen_viewport.TopLeftY = 0;
    this->screen_viewport.Width = static_cast<float>(client_width);
    this->screen_viewport.Height = static_cast<float>(client_height);
    this->screen_viewport.MinDepth = 0.F;
    this->screen_viewport.MaxDepth = 1.F;

    context->RSSetViewports(1, &this->screen_viewport);
    return child;
}
