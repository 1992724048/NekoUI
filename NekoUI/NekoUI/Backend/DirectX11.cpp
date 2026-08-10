// 2026-08-10 12:53:08

#ifdef _WIN32
#include "DirectX11.hpp"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

namespace neko::backend {
    DirectX11::DirectX11(const HWND hwnd) :
        surface_{hwnd} {}
} // namespace neko::backend
#endif
