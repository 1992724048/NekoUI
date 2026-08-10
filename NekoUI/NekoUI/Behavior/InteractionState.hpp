// 2026-08-10

#pragma once

#include <atomic>

namespace neko::behavior {
    struct InteractionState {
        std::atomic_bool hovered{false};
    };
}
