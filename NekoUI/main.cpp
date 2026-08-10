// 2026-08-10 11:23:04

#include <Windows.h>
#include <iostream>
#include <memory>
#include <print>
#include <string>

#include "NekoUI/Type.hpp"
#include "NekoUI/Backend/DirectX11.hpp"
#include "NekoUI/Engine/Core/Engine.hpp"
#include "NekoUI/Platform/Win32.hpp"
#include "NekoUI/Widget/Button/Button.hpp"
#include "NekoUI/Widget/Layout/Column/Column.hpp"
#include "NekoUI/Widget/TextField/TextField.hpp"

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
    constexpr std::wstring class_name = L"NekoUI";

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

    HWND hwnd = CreateWindowW(class_name.data(), L"NekoUI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    win32 = std::make_unique<neko::platform::Win32>();
    win32->set_window(hwnd);

    auto directx11 = std::make_unique<neko::backend::DirectX11>(hwnd);
    engine = std::make_unique<neko::engine::Engine>(std::move(directx11));
    engine->get_context().set_ime_pos = std::bind(&neko::platform::Win32::set_ime_window_position, win32.get(), std::placeholders::_1, std::placeholders::_2);
    msg_pump = engine->get_msg_pump();
    if (const auto pump = msg_pump.lock()) {
        pump->push_msg(win32->query_theme());
    }

    bool show_field = true;
    std::string input_text;
    engine->set_root_widget<neko::widget::Column>([&](neko::widget::Widget& root) -> void {
        auto& column = static_cast<neko::widget::Column&>(root);
        column.children([&](neko::widget::Widget& builder) -> void {
            const auto b1 = builder.build<neko::widget::Button>("Button 1");
            b1->style().size.value = {.x = 120.0F, .y = 40.0F};
            b1->on_click([]() -> void {
                std::println("Button 1 clicked!");
            });

            const auto b2 = builder.build<neko::widget::Button>("Button 2");
            b2->style().size.value = {.x = 120.0F, .y = 40.0F};
            b2->on_click([]() -> void {
                std::println("Button 2 clicked!");
            });

            const auto b3 = builder.build<neko::widget::Button>("Button 3");
            b3->style().size.value = {.x = 120.0F, .y = 40.0F};
            b3->on_click([]() -> void {
                std::println("Button 3 clicked!");
            });

            const auto toggle_btn = builder.build<neko::widget::Button>("切换输入框");
            toggle_btn->style().size.value = {.x = 120.0F, .y = 40.0F};
            toggle_btn->on_click([&]() -> void {
                show_field = !show_field;
                engine->rebuild();
            });

            if (show_field) {
                builder.build<neko::widget::TextField>(input_text)->style().size.value = {.x = 240.0F, .y = 40.0F};

                const auto show_btn = builder.build<neko::widget::Button>("显示输入");
                show_btn->style().size.value = {.x = 120.0F, .y = 40.0F};
                show_btn->on_click([&]() -> void {
                    std::println("输入内容: {}", input_text);
                });
            }
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
