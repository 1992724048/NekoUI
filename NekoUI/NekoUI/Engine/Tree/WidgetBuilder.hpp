// 2026-08-10 10:17:46

#pragma once
#include "TreeManager.hpp"

namespace neko::engine {
    struct Context;

    class WidgetBuilder {
    public:
        explicit WidgetBuilder(std::weak_ptr<TreeManager> tree);
        auto build(Context& context) -> void;
    private:
        std::weak_ptr<TreeManager> tree_;
    };
} // namespace neko::engine
