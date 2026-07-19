// 2026-07-20 02:16:37

#pragma once
#include <memory>
#include <optional>

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
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> std::optional<std::weak_ptr<widget::Widget>>;
    private:
        TreeManager& tree_;
    };
} // namespace neko::engine
