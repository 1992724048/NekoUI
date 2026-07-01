#pragma once
#include <functional>
#include <memory>
#include <vector>

namespace neko::engine {
    struct Context {
        std::function<void()> rebuild;
        std::function<void()> rerender;
        std::function<void()> animation_start;
        std::function<void()> animation_end;
    };
} // namespace neko::engine
