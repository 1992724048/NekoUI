#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <print>
#include <string>

#include "NekoUI/Engine/Engine.hpp"

static auto msg_proc(const HWND hwnd, const UINT msg, const WPARAM param1, const LPARAM param2) -> LRESULT {
    return DefWindowProcW(hwnd, msg, param1, param2);
}

auto main(int argc, char* argv[]) -> int try {
    std::wstring class_name(L"TestUI");

    WNDCLASS win_class{};
    win_class.lpszClassName = class_name.data();
    win_class.hInstance = GetModuleHandleW(nullptr);
    win_class.lpfnWndProc = msg_proc;
    win_class.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClassW(&win_class) == 0U) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    const HWND hwnd = CreateWindowW(class_name.data(), L"NekoUI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    neko::engine::Engine engine(hwnd);

    return 0;
} catch (const std::exception& error) {
    std::cout << error.what();
}
