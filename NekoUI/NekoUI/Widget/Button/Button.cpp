#include "Button.hpp"

using namespace neko::type;

neko::widget::Button::Button(Widget* parent) : Widget{parent} {}

neko::widget::Button::Button(engine::Context& context, const Vec4I bounds, const std::string_view /*label*/) :
    Widget{context} {
    this->bounds = bounds;
}
