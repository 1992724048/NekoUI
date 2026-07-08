#include <Windows.h>
#include <iostream>
#include <print>
#include <string>

#include "NekoUI/Engine/Engine.hpp"
#include "NekoUI/Widget/Button/Button.hpp"
#include "NekoUI/Widget/Checkbox/Checkbox.hpp"
#include "NekoUI/Widget/TextInput/TextInput.hpp"

namespace {
    auto msg_proc(const HWND hwnd, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> LRESULT {
        switch (msg) {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_GETMINMAXINFO: {
                auto* mmi = reinterpret_cast<MINMAXINFO*>(lparam);
                mmi->ptMinTrackSize = {.x = 200, .y = 150};
                return 0;
            }
            default:
                break;
        }

        if (auto* engine = reinterpret_cast<neko::engine::Engine*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA))) {
            switch (msg) {
                case WM_SIZE:
                case WM_DPICHANGED:
                case WM_MOUSEMOVE:
                case WM_LBUTTONDOWN:
                case WM_LBUTTONUP:
                case WM_RBUTTONDOWN:
                case WM_RBUTTONUP:
                case WM_MBUTTONDOWN:
                case WM_MBUTTONUP:
                case WM_MOUSEWHEEL:
                case WM_KEYDOWN:
                case WM_KEYUP:
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_CHAR:
                case WM_TIMER:
                    engine->push_msg(msg, wparam, lparam);
                    break;
                case WM_SETCURSOR: {
                    if (LOWORD(lparam) == HTCLIENT) {
                        POINT pt;
                        GetCursorPos(&pt);
                        ScreenToClient(hwnd, &pt);
                        SetCursor(LoadCursorW(GetModuleHandleW(nullptr), engine->has_interactive_at(pt) ? MAKEINTRESOURCEW(32649) : MAKEINTRESOURCEW(32512)));
                        return TRUE;
                    }
                    break;
                }
                default: ;
            }
        }

        return DefWindowProcW(hwnd, msg, wparam, lparam);
    }
} // namespace

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

    const HWND hwnd = CreateWindowW(class_name.data(), L"NekoUI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, nullptr, nullptr, win_class.hInstance, nullptr);
    if (hwnd == nullptr) {
        std::println("Error {:#X}", GetLastError());
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    neko::engine::Engine engine(hwnd);

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&engine));

    auto& btn = engine.set<neko::widget::Button>(glm::ivec4{100, 100, 200, 50}, "点我");
    btn.on_click = [] -> void {
        std::println("[NekoUI] Button clicked!\n");
    };

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) != 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return static_cast<int>(msg.wParam);
} catch (const std::exception& error) {
    std::cout << error.what();
}
