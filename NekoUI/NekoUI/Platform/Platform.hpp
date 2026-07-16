#pragma once
#include <memory>
#include <optional>

#include "Event.hpp"

namespace neko::platform {
    struct NativeMessage;
    struct NativeWindow;

    class Platform {
    public:
        Platform(const Platform& other) = delete;
        Platform(Platform&& other) noexcept = delete;
        auto operator=(const Platform& other) -> Platform& = delete;
        auto operator=(Platform&& other) noexcept -> Platform& = delete;

        Platform() = default;
        virtual ~Platform() = default;

        [[nodiscard]] virtual auto translate_event(const NativeMessage& msg) const -> std::optional<Event> {
            return std::nullopt;
        }

        [[nodiscard]] virtual auto query_theme() const -> ThemeChangedEvent {
            return {.mode = ThemeMode::Light, .color = type::Color{.value = 0}};
        }

        [[nodiscard]] virtual auto activate_ime(const NativeWindow& native_window, bool active) const -> bool {
            return false;
        }

        static auto instance() -> const Platform& {
            return *platform;
        }
    protected:
        template<typename T>
        friend auto register_platform() noexcept -> void;
        static std::unique_ptr<Platform> platform;
    };

    template<typename T>
    auto register_platform() noexcept -> void {
        Platform::platform = std::make_unique<T>();
    }

    inline std::unique_ptr<Platform> Platform::platform{nullptr};
}

// 用法：在平台实现的 .cpp 文件中，namespace neko::platform { ... } 内调用
// NEKO_REGISTER_PLATFORM(Win32)
#define NEKO_REGISTER_PLATFORM(T)                                             \
    namespace {                                                               \
        [[maybe_unused]] const auto kNEKO_REGISTER_PLATFORM_##T = [] {         \
            ::neko::platform::register_platform<T>();                          \
            return true;                                                       \
        }();                                                                   \
    }
