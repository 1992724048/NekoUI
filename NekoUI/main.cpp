// 2026-08-10 04:48:52

#include <Windows.h>
#include <iostream>
#include <print>
#include <string>

#include "NekoUI/Type.hpp"
#include "NekoUI/Backend/DirectX11.hpp"
#include "NekoUI/Engine/Core/Engine.hpp"
#include "NekoUI/Platform/Win32.hpp"
#include "NekoUI/Widget/Button/Button.hpp"
#include "NekoUI/Widget/Layout/Column/Column.hpp"

using namespace neko::type;

namespace {
    std::unique_ptr<neko::engine::Engine> engine;
    std::weak_ptr<neko::engine::MsgPump> msg_pump;
    std::unique_ptr<neko::platform::Win32> win32;
}

namespace {
    auto msg_proc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> LRESULT {
        switch (msg) {
            case WM_DESTROY:
                PostQuitMessage(0);
                break;
            case WM_GETMINMAXINFO: {
                auto* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
                mmi->ptMinTrackSize = {.x = 200, .y = 150};
                return 0;
            }
            default:
                break;
        }

        if (engine && win32) {
            win32->handle_message(msg, wparam, lparam, msg_pump);
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
}

auto main(int argc, char* argv[]) -> int try {
    const std::wstring class_name = L"NekoUI";

    WNDCLASSW win_class{};
    win_class.lpszClassName = class_name.data();
    win_class.hInstance = GetModuleHandleW(nullptr);
    win_class.lpfnWndProc = msg_proc;
    win_class.hCursor = LoadCursorW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(32512));
    win_class.style = CS_HREDRAW | CS_VREDRAW;

    if (RegisterClassW(&win_class) == 0U) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    win32 = std::make_unique<neko::platform::Win32>();

    HWND hwnd = CreateWindowW(class_name.data(), L"NekoUI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    auto directx11 = std::make_unique<neko::backend::DirectX11>(hwnd);
    engine = std::make_unique<neko::engine::Engine>(std::move(directx11));
    msg_pump = engine->get_msg_pump();
    if (const auto pump = msg_pump.lock()) {
        pump->push_msg(win32->query_theme());
    }

    const auto page = engine->set_root_widget<neko::widget::Column>();

    page->children([&](auto& col) -> auto {
        auto& b1 = col.template build<neko::widget::Button>("Button 1");
        b1.style().size.value = {.x = 120.0F, .y = 40.0F};
        b1.on_click([] -> auto {
            std::println("Button 1 clicked!");
        });

        auto& b2 = col.template build<neko::widget::Button>("Button 2");
        b2.style().size.value = {.x = 120.0F, .y = 40.0F};
        b2.on_click([] -> auto {
            std::println("Button 2 clicked!");
        });

        auto& b3 = col.template build<neko::widget::Button>("Button 3");
        b3.style().size.value = {.x = 120.0F, .y = 40.0F};
        b3.on_click([] -> auto {
            std::println("Button 3 clicked!");
        });
    });

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) != 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
} catch (const std::exception& error) {
    std::cout << error.what();
}
