#pragma once
#include "../Type.hpp"

namespace neko::device {
    struct Mouse;
}

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    class TreeManager;

    class HitTester {
    public:
        explicit HitTester(TreeManager& tree);
        auto hit_test(const device::Mouse& mouse) const -> widget::Widget*;
    private:
        TreeManager& tree_;
    };
} // namespace neko::engine
