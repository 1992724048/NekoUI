#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>
#include <string_view>

#include "../../Engine/Engine.hpp"
#include "../../Engine/Component/Animation.hpp"

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(Widget* parent);
        explicit Button(engine::Engine* engine, Vec4I bounds, std::string_view label);
    private:
        animation::Animation<int, animation::ease::quad::in_out> color{0, 200};
    };
} // namespace neko::widget
