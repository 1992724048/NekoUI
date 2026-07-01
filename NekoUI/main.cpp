#include <Windows.h>
#include <cstdio>
#include <iostream>
#include <print>
#include <string>

#include "NekoUI/Engine/Engine.hpp"
#include "NekoUI/Widget/Component/Rect.hpp"

static auto msg_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_GETMINMAXINFO: {
            auto* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
            mmi->ptMinTrackSize = {200, 150};
            return 0;
        }
        default:
            break;
    }

    if (auto* engine = reinterpret_cast<neko::engine::Engine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
        switch (msg) {
            case WM_SIZE:
            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            case WM_CHAR:
                engine->push_msg(msg, wparam, lparam);
                break;
        }
    }

    return DefWindowProcW(hwnd, msg, wparam, lparam);
}

auto main(int argc, char* argv[]) -> int try {
    std::wstring class_name(L"NekoUI");

    WNDCLASSW win_class{};
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

    // 关联 Engine 到窗口，wndproc 通过 GWLP_USERDATA 获取
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&engine));

    // 绘制一个矩形控件
    auto rect = std::make_shared<neko::widget::RectWidget>(glm::ivec4{100, 100, 200, 150}, neko::type::Color{255, 100, 100, 255});
    engine.set_widget(rect);

    // 消息循环
    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
} catch (const std::exception& error) {
    std::cout << error.what();
}
