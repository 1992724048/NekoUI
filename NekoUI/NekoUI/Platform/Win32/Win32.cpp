#include "Win32.hpp"
#ifdef _WIN32
#include <dwmapi.h>
#include <string_view>
#pragma comment(lib, "dwmapi.lib")

namespace {
    auto query_theme() -> neko::platform::ThemeChangedEvent {
        auto mode = neko::platform::ThemeMode::Light;
        HKEY key{};
        if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &key) == ERROR_SUCCESS) {
            DWORD value{};
            DWORD size = sizeof(value);
            if (RegQueryValueExW(key, L"AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
                mode = value != 0 ? neko::platform::ThemeMode::Light : neko::platform::ThemeMode::Dark;
            }
            RegCloseKey(key);
        }
        DWORD raw{};
        if (FAILED(DwmGetColorizationColor(&raw, nullptr))) {
            raw = 0;
        }
        return neko::platform::ThemeChangedEvent{.mode = mode, .color = neko::type::Color{.value = raw}};
    }
}

namespace neko::platform {
    NEKO_REGISTER_PLATFORM(Win32)

    Win32::Win32() {
        cached_theme_ = ::query_theme();
    }

    auto Win32::translate_event(const NativeMessage& nm) const -> std::optional<Event> {
        const UINT msg = nm.msg;
        const WPARAM wparam = nm.wparam;
        const LPARAM lparam = nm.lparam;

        switch (msg) {
            case WM_MOUSEMOVE:
                return device::MouseMoveEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)),};
            case WM_LBUTTONDOWN:
                return device::MouseButtonEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)), .button = device::MouseButton::Left, .pressed = true,};
            case WM_LBUTTONUP:
                return device::MouseButtonEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)), .button = device::MouseButton::Left, .pressed = false,};
            case WM_RBUTTONDOWN:
                return device::MouseButtonEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)), .button = device::MouseButton::Right, .pressed = true,};
            case WM_RBUTTONUP:
                return device::MouseButtonEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)), .button = device::MouseButton::Right, .pressed = false,};
            case WM_MBUTTONDOWN:
                return device::MouseButtonEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)), .button = device::MouseButton::Middle, .pressed = true,};
            case WM_MBUTTONUP:
                return device::MouseButtonEvent{.x = static_cast<int>(LOWORD(lparam)), .y = static_cast<int>(HIWORD(lparam)), .button = device::MouseButton::Middle, .pressed = false,};
            case WM_MOUSEWHEEL:
                return device::MouseWheelEvent{.delta = GET_WHEEL_DELTA_WPARAM(wparam),};
            case WM_KEYDOWN:
            case WM_SYSKEYDOWN:
                return device::KeyEvent{.key = static_cast<int>(wparam), .pressed = true,};
            case WM_KEYUP:
            case WM_SYSKEYUP:
                return device::KeyEvent{.key = static_cast<int>(wparam), .pressed = false,};
            case WM_CHAR:
                return device::CharEvent{.ch = static_cast<wchar_t>(wparam),};
            case WM_SIZE:
                return ResizeEvent{.width = static_cast<int>(LOWORD(lparam)), .height = static_cast<int>(HIWORD(lparam)),};
            case WM_DPICHANGED:
                return DpiChangeEvent{.dpi = static_cast<uint32_t>(LOWORD(wparam)),};
            case WM_CREATE:
                return this->query_theme();
            case WM_DESTROY:
                return DestroyEvent{};
            case WM_SETTINGCHANGE: {
                if (lparam != 0 && std::wstring_view{reinterpret_cast<const wchar_t*>(lparam)} == L"ImmersiveColorSet") {
                    cached_theme_ = ::query_theme();
                    return cached_theme_;
                }
                return std::nullopt;
            }
            default:
                return std::nullopt;
        }
    }

    auto Win32::query_theme() const -> ThemeChangedEvent {
        return cached_theme_;
    }

    Win32::~Win32() {
        if (ime_doc_mgr_) {
            ime_doc_mgr_->Release();
        }
        if (ime_thread_mgr_) {
            ime_thread_mgr_->Deactivate();
            ime_thread_mgr_->Release();
        }
        if (ime_com_initialized_) {
            CoUninitialize();
        }
    }

    auto Win32::init_ime() const -> void {
        if (ime_initialized_) { return; }
        ime_initialized_ = true;

        const HRESULT com_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ime_com_initialized_ = (com_hr == S_OK);

        if (FAILED(CoCreateInstance(CLSID_TF_ThreadMgr, nullptr, CLSCTX_INPROC_SERVER, IID_ITfThreadMgr,
            reinterpret_cast<void**>(&ime_thread_mgr_)))) {
            return;
        }

        if (FAILED(ime_thread_mgr_->Activate(&ime_client_id_))) {
            return;
        }

        ime_thread_mgr_->CreateDocumentMgr(&ime_doc_mgr_);
    }

    auto Win32::activate_ime(void* native_window, bool active) const -> bool {
        init_ime();
        if (!ime_thread_mgr_) { return false; }

        if (active) {
            if (ime_doc_mgr_) {
                ITfContext* context{};
                if (SUCCEEDED(ime_thread_mgr_->CreateContext(ime_client_id_, 0, nullptr, &context, nullptr))) {
                    ime_doc_mgr_->Push(context);
                    context->Release();
                }
                ime_thread_mgr_->SetFocus(ime_doc_mgr_);
            }
        } else {
            ime_thread_mgr_->SetFocus(nullptr);
            if (ime_doc_mgr_) {
                ime_doc_mgr_->Pop(TF_POPF_ALL);
            }
        }
        return true;
    }
}
#endif
