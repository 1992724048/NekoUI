#include "Windows.hpp"

#include <print>
#include <stdexcept>
#include <windowsx.h>

using namespace neko::window::impl;

Windows::Windows(const HWND hwnd) {
    this->hwnd = hwnd;
}

Windows::~Windows() noexcept = default;

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

    const HWND hwnd = CreateWindowW(class_name.data(), title.data(), WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return nullptr;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    const auto ins = std::make_shared<Windows>(hwnd);
    ins->proc = proc;
    ins->state.window.window_size = {.x = 800, .y = 600};

    std::unique_lock _(mutex);
    return window_list[hwnd] = ins;
}

auto Windows::create(HWND hwnd) -> std::shared_ptr<Windows> {
    std::unique_lock _(mutex);
    auto& ins = window_list[hwnd] = std::make_shared<Windows>(hwnd);
    ins->state.window.first_create = false;

    RECT rect;
    GetWindowRect(hwnd, &rect);
    ins->state.window.window_size = {.x = rect.right - rect.left, .y = rect.bottom - rect.top};
    return ins;
}

auto Windows::get_msg() -> void {
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    std::unique_lock _(mutex);
    window_list.clear();
}

auto Windows::msg_proc(const HWND hwnd, const UINT msg, const WPARAM param1, const LPARAM param2) -> LRESULT {
    std::shared_lock read_lock(mutex);

    if (!window_list.contains(hwnd)) {
        return DefWindowProcW(hwnd, msg, param1, param2);
    }

    const auto& ins = window_list[hwnd];
    msg_transform(ins->state, msg, param1, param2);

    if (!ins->msg_callback) {
        goto default_flow;
    }

    if (ins->msg_callback() == MsgResult::Ignore) {
        goto default_flow;
    }

    if (ins->state.window.destroy) {
        window_list.erase(hwnd);
        return 0;
    }

    ins->state.mouse.keys.reset(static_cast<size_t>(MouseKey::WheelUp));
    ins->state.mouse.keys.reset(static_cast<size_t>(MouseKey::WheelDown));
    return 0;

default_flow:
    if (ins->proc) {
        return ins->proc(hwnd, msg, param1, param2);
    }
    return DefWindowProcW(hwnd, msg, param1, param2);
}

auto Windows::msg_transform(InputState& state, const UINT msg, const WPARAM param1, const LPARAM param2) -> void {
    switch (msg) {
        case WM_DESTROY: {
            state.window.destroy = true;
            break;
        }
        case WM_CREATE: {
            state.window.first_create = true;
            break;
        }
        case WM_SIZE: {
            state.window.window_size.x = LOWORD(param2);
            state.window.window_size.y = HIWORD(param2);
            state.window.resized = true;
            break;
        }
        case WM_MOUSEMOVE: {
            state.mouse.x = GET_X_LPARAM(param2);
            state.mouse.y = GET_Y_LPARAM(param2);
            break;
        }
        case WM_LBUTTONDOWN: {
            state.mouse.keys.set(static_cast<size_t>(MouseKey::LeftButton));
            break;
        }
        case WM_LBUTTONUP: {
            state.mouse.keys.reset(static_cast<size_t>(MouseKey::LeftButton));
            break;
        }
        case WM_RBUTTONDOWN: {
            state.mouse.keys.set(static_cast<size_t>(MouseKey::RightButton));
            break;
        }
        case WM_RBUTTONUP: {
            state.mouse.keys.reset(static_cast<size_t>(MouseKey::RightButton));
            break;
        }
        case WM_MBUTTONDOWN: {
            state.mouse.keys.set(static_cast<size_t>(MouseKey::MiddleButton));
            break;
        }
        case WM_MBUTTONUP: {
            state.mouse.keys.reset(static_cast<size_t>(MouseKey::MiddleButton));
            break;
        }
        case WM_MOUSEWHEEL: {
            const auto delta = GET_WHEEL_DELTA_WPARAM(param1);
            state.mouse.wheel += delta;
            if (delta > 0) {
                state.mouse.keys.set(static_cast<size_t>(MouseKey::WheelUp));
            } else {
                state.mouse.keys.set(static_cast<size_t>(MouseKey::WheelDown));
            }

            break;
        }
        default: ;
    }
}
