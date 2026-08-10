// 2026-08-10 12:53:48

#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>

#include "../Type.hpp"

namespace neko::backend {
    using namespace neko::type;

    class Surface final {
    public:
        explicit Surface(HWND hwnd);
        ~Surface();

        Surface(const Surface&) = delete;
        auto operator=(const Surface&) -> Surface& = delete;

        auto resize(Vec2I new_size) -> void;
        auto set_dpi(unsigned int dpi) -> void;
        [[nodiscard]] auto get_dpi_scale() const -> float;
        auto begin_rt() const -> void;
        auto end() const -> void;
        [[nodiscard]] auto device() const -> ID3D11Device*;
        [[nodiscard]] auto context() const -> ID3D11DeviceContext*;
        [[nodiscard]] auto size() const -> Vec2I;
        [[nodiscard]] auto native_handle() const -> Handle;
        [[nodiscard]] auto client_size() const -> Vec2I;
    private:
        ID3D11Device* device_{};
        ID3D11DeviceContext* ctx_{};
        IDXGISwapChain1* swap_chain_{};
        ID3D11RenderTargetView* rtv_{};
        Vec2I size_{};
        float dpi_scale_ = 1.0F;
        HWND hwnd_{};

        auto init_device() -> void;
        auto init_swap_chain(HWND hwnd) -> void;
    };
}
#endif
