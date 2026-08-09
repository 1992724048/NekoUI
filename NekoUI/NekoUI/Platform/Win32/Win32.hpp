#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <memory>
#include <msctf.h>
#include <optional>

#include "../Event.hpp"
#include "../../Engine/MsgPump.hpp"

namespace neko::platform {
    struct NativeMessage {
        UINT msg;
        WPARAM wparam;
        LPARAM lparam;
    };

    class Win32 final {
    public:
        Win32();
        ~Win32();
        [[nodiscard]] auto translate_event(const NativeMessage& nm) -> std::optional<Event>;
        [[nodiscard]] auto query_theme() -> ThemeChangedEvent;
        [[nodiscard]] auto activate_ime(type::Handle native_window, bool active) -> bool;
        auto show_window(type::Handle native_window) -> void;
        auto hide_window(type::Handle native_window) -> void;
        auto close_window(type::Handle native_window) -> void;
        auto maximize_window(type::Handle native_window) -> void;
        auto minimize_window(type::Handle native_window) -> void;
        auto restore_window(type::Handle native_window) -> void;
        auto destroy_window(type::Handle native_window) -> void;
        auto move_window(type::Handle native_window, int x, int y) -> void;
        auto resize_window(type::Handle native_window, int width, int height) -> void;
        auto set_focus(type::Handle native_window) -> void;
        auto set_opacity(type::Handle native_window, float opacity) -> void;
    private:
        ThemeChangedEvent cached_theme_{};
        ITfThreadMgr* ime_thread_mgr_{};
        ITfDocumentMgr* ime_doc_mgr_{};
        DWORD ime_client_id_{};
        bool ime_initialized_{};
        bool ime_com_initialized_{};
        auto init_ime() -> void;
    public:
        auto handle_message(const UINT msg, const WPARAM wparam, const LPARAM lparam, const std::weak_ptr<engine::MsgPump>& pump) -> bool {
            if (const auto p = pump.lock()) {
                const NativeMessage native{.msg = msg, .wparam = wparam, .lparam = lparam};
                if (const auto event = translate_event(native)) {
                    p->push_msg(*event);
                    return true;
                }
            }
            return false;
        }
    };
}
#endif
