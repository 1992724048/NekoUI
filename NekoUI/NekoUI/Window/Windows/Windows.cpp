#include "Windows.hpp"

#include <print>
#include <stdexcept>

using namespace neko::window::impl;

Windows::Windows(const HWND hwnd) {
    this->hwnd = hwnd;
}

Windows::~Windows() noexcept {
    CloseWindow(hwnd);
    DestroyWindow(hwnd);

    std::unique_lock _(mutex);
    window_list.erase(hwnd);
}

auto Windows::get_handle() -> Handle {
    return this->hwnd;
}

auto Windows::create(const std::wstring_view title, const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& proc) -> std::shared_ptr<Windows> {
    std::wstring class_name(title);
    class_name += L"_class";

    WNDCLASS win_class{};
    win_class.lpszClassName = class_name.data();
    win_class.hInstance = GetModuleHandleW(nullptr);
    win_class.lpfnWndProc = msg_proc;
    win_class.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClassW(&win_class) == 0U) {
        std::println("Error {:#X}", GetLastError());
        return nullptr;
    }

    const HWND hwnd = CreateWindowW(class_name.data(), title.data(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return nullptr;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    const auto ins = std::make_shared<Windows>(hwnd);
    ins->proc = proc;

    std::unique_lock _(mutex);
    return window_list[hwnd] = ins;
}

auto Windows::create(HWND hwnd) -> std::shared_ptr<Windows> {
    std::unique_lock _(mutex);
    return window_list[hwnd] = std::make_shared<Windows>(hwnd);
}

auto Windows::get_msg() -> void {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (msg.message == WM_QUIT) {
            return;
        }

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

auto Windows::msg_proc(const HWND hwnd, const UINT msg, const WPARAM param1, const LPARAM param2) -> LRESULT {
    std::shared_lock _(mutex);

    if (window_list.contains(hwnd)) {
        const auto& ins = window_list[hwnd];
        if (ins->msg_callback) {
            constexpr InputMsg msg_{};
            constexpr InputMsgType msg_type{};
            if (ins->msg_callback(Mouse, msg_type, msg_) == MsgResult::Ignore) {
                if (ins->proc) {
                    return ins->proc(hwnd, msg, param1, param2);
                }
                return 0;
            }
        }
    }

    return DefWindowProcW(hwnd, msg, param1, param2);
}
