#pragma once
#ifdef _WIN32
#include <Windows.h>
#include <optional>

#include "../Platform.hpp"

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
    };
}
#endif
