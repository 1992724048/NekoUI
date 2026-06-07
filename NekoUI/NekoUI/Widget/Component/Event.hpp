#pragma once
#include "../../Type.hpp"

namespace neko::event {
    using namespace type;

    class Event {
    public:
        virtual ~Event() = default;

        virtual auto event_range() -> Vec4<int> {
            return {.x = 0, .y = 0, .z = 0, .w = 0};
        }
    };
} // namespace neko::event
