#pragma once
#include <functional>
#include <memory>
#include <vector>

namespace neko::widget {
    class Widget;
} // namespace neko::widget

namespace neko::engine {
    using namespace type;

    struct Context {
        std::function<void()> set_state;
    };
} // namespace neko::engine
