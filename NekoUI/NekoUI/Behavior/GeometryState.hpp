// 2026-08-10

#pragma once

#include "../Type.hpp"

namespace neko::behavior {
    using namespace neko::type;

    // 行为间共享的几何状态（布局写、绘制/命中/输入读；生命周期 = 持有控件）
    struct GeometryState {
        Vec4I bounds{};
    };
}
