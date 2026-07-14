#include "Button.hpp"

neko::widget::Button::Button(Widget* parent) : Widget{parent} {}

neko::widget::Button::Button(engine::Engine* engine, const glm::ivec4 bounds, const std::string_view /*label*/) : Widget{engine} {
    this->bounds = bounds;
}
