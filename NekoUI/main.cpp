#include "NekoUI/Backend/DirectX11/DirectX11.hpp"
#include "NekoUI/Engine/Engine.hpp"
#include "NekoUI/Window/Windows/Windows.hpp"

static auto msg_proc(const HWND hwnd, const UINT msg, const WPARAM param1, const LPARAM param2) -> LRESULT {
    if (msg == WM_CLOSE) {
        return DestroyWindow(hwnd);
    }
    return DefWindowProcW(hwnd, msg, param1, param2);
}

auto main(int argc, char* argv[]) -> int {
    using namespace neko;
    using namespace backend::impl;

    const auto window_ptr = window::impl::Windows::create(L"test", msg_proc);
    const auto backend_ptr = DirectX11::create(window_ptr);

    const auto window_ptr2 = window::impl::Windows::create(L"test2", msg_proc);
    const auto backend_ptr2 = DirectX11::create(window_ptr2);

    const auto engine = std::make_unique<engine::Engine>();
    engine->add(window_ptr, backend_ptr);
    engine->add(window_ptr2, backend_ptr2);

    window::impl::Windows::get_msg();
    return 0;
}
