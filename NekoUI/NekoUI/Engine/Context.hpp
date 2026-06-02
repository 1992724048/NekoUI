#pragma once
#include <memory>
#include <vector>

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    using namespace type;

    struct Context {
        Vec4<int> body_range;
        std::vector<std::shared_ptr<widget::Widget>> stack;
    };
} // namespace neko::engine
