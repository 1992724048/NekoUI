#pragma once
#include "../Widget.hpp"
#include "../Component/Mouse.hpp"

namespace neko::widget {
    using namespace event;

    class Button final : public Widget, Mouse {
    public:
        auto mouse_move(Vec2<int> pos) -> void override;
        auto mouse_button(Vec2<int> pos, std::bitset<7> state) -> void override;
        auto mouse_wheel(Vec2<int> pos, int wheel) -> void override;

        auto draw(engine::Context context) -> void override;
    };
} // namespace neko::widget
