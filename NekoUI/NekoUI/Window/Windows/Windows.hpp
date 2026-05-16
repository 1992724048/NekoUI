#pragma once
#include <Windows.h>
#include <memory>
#include <shared_mutex>
#include <string_view>
#include <unordered_map>
#include <windef.h>

#include "../Window.hpp"

namespace neko::window::impl {
    class Windows final : public Window {
    public:
        explicit Windows(HWND hwnd);
        ~Windows() noexcept override;
        auto get_handle() -> Handle override;

        static auto create(std::wstring_view title, const std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)>& proc) -> std::shared_ptr<Windows>;
        static auto create(HWND hwnd) -> std::shared_ptr<Windows>;

        static auto get_msg() -> void;
        static auto msg_proc(HWND hwnd, UINT msg, WPARAM param1, LPARAM param2) -> LRESULT;
    private:
        HWND hwnd{};
        std::function<LRESULT(HWND, UINT, WPARAM, LPARAM)> proc;

        inline static std::shared_mutex mutex;
        inline static std::unordered_map<HWND, std::shared_ptr<Windows>> window_list;
    };
} // namespace neko::window::impl
