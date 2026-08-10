// 2026-08-10 10:17:41

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
        explicit HitTester(std::weak_ptr<TreeManager> tree);
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> std::optional<std::shared_ptr<widget::Widget>>;
    private:
        std::weak_ptr<TreeManager> tree_;
    };
} // namespace neko::engine
