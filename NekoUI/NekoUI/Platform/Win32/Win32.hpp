#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <memory>
#include <optional>

#include "../Platform.hpp"
#include "../../Engine/MsgPump.hpp"

namespace neko::platform {
    struct NativeMessage {
        UINT msg;
        WPARAM wparam;
        LPARAM lparam;
    };

    class Win32 final : public Platform {
    public:
        Win32();
        [[nodiscard]] auto translate_event(const NativeMessage& nm) const -> std::optional<Event> override;
        [[nodiscard]] auto query_theme() const -> ThemeChangedEvent override;

        static auto handle_message(const UINT msg, const WPARAM wparam, const LPARAM lparam, const std::weak_ptr<engine::MsgPump>& pump) -> bool {
            if (const auto p = pump.lock()) {
                const NativeMessage native{.msg = msg, .wparam = wparam, .lparam = lparam};
                if (const auto event = instance().translate_event(native)) {
                    p->push_msg(*event);
                    return true;
                }
            }
            return false;
        }
    };
}
#endif
