#pragma once
#include "../Backend.hpp"

#include <Windows.h>

namespace neko::backend::impl {
    class DirectX11 final : public Backend {
    public:
        static auto create(const std::shared_ptr<window::Window>& window) -> std::shared_ptr<DirectX11>;
    };
} // namespace neko::backend::impl
