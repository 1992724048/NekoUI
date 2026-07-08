#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>
#include <string_view>

#include "../../Engine/Component/Animation.hpp"
#include "../../Engine/Engine.hpp"

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(Widget* parent);
        explicit Button(engine::Engine* engine, glm::ivec4 bounds, std::string_view label);
    private:
        animation::LinearAnimation<int> color{0, 200};
    };
} // namespace neko::widget
