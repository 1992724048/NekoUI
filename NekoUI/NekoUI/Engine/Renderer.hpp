#pragma once
#include <cstdint>
#include "../Type.hpp"

namespace neko::backend {
    class Backend;
}

namespace neko::engine {
    struct Context;
    class TreeManager;

    class Renderer {
    public:
        explicit Renderer(TreeManager& tree);
        auto render(type::Vec4I rect, Context& context, backend::Backend& backend) const -> void;
    private:
        TreeManager& tree_;
    };
} // namespace neko::engine
