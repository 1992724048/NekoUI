#pragma once
#include <memory>
#include "../Type.hpp"

namespace neko::device { struct Mouse; }
namespace neko::widget { class Widget; }

namespace neko::engine {
    class TreeManager;

    class HitTester {
    public:
        explicit HitTester(TreeManager& tree);
        /// 返回命中的最上层 Widget（shared_ptr 保证生命周期），无命中返回 nullptr
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> std::shared_ptr<widget::Widget>;
    private:
        TreeManager& tree_;
    };
} // namespace neko::engine
