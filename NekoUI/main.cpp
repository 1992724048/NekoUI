#include "NekoUI/Backend/DirectX11/DirectX11.hpp"
#include "NekoUI/Engine/Engine.hpp"
#include "NekoUI/Window/Windows/Windows.hpp"

auto msg_proc(const HWND hwnd, const UINT msg, const WPARAM param1, const LPARAM param2) -> LRESULT {
    if (msg == WM_CLOSE) {
        PostQuitMessage(0);
    }

    return DefWindowProcW(hwnd, msg, param1, param2);
}

auto main(int argc, char* argv[]) -> int {
    using namespace neko;
    using namespace window::impl;
    using namespace backend::impl;

    const auto window_ptr = Windows::create(L"test", msg_proc);
    const auto backend_ptr = DirectX11::create(window_ptr);

    const auto engine = std::make_unique<engine::Engine>();
    engine->add(window_ptr, backend_ptr);

    Windows::get_msg();
    return 0;
}
