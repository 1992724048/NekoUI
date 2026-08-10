// 2026-08-10

#pragma once

#include <atomic>

namespace neko::behavior {
    // 行为间共享的交互状态（Input 写、Draw 读；生命周期 = 持有控件）
    struct InteractionState {
        std::atomic_bool hovered{false};
    };
}
