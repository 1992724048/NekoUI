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
        std::function<void()> rebuild;
        std::function<void()> rerender;
        std::function<void()> animation_start;
        std::function<void()> animation_end;
    };
} // namespace neko::engine
