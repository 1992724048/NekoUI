#pragma once
#include "TreeManager.hpp"

namespace neko::engine {
    struct Context;

    class WidgetBuilder {
    public:
        explicit WidgetBuilder(TreeManager& tree);
        auto build(Context& context) -> void;

    private:
        TreeManager& tree_;
    };
} // namespace neko::engine
