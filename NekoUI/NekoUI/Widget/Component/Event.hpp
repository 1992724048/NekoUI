#pragma once
#include "../../Type.hpp"

namespace neko::event {
    using namespace type;

    class Event {
    public:
        virtual ~Event() = default;

        virtual auto is_hovered(Vec2<int> mouse_pos) -> bool {
            return false;
        }
    };
} // namespace neko::event
