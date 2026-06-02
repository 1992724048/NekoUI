#include "Button.hpp"
using namespace neko::widget;

auto Button::mouse_move(Vec2<int> pos) -> void {
    Mouse::mouse_move(pos);
}

auto Button::mouse_button(Vec2<int> pos, std::bitset<7> state) -> void {
    Mouse::mouse_button(pos, state);
}

auto Button::mouse_wheel(Vec2<int> pos, int wheel) -> void {
    Mouse::mouse_wheel(pos, wheel);
}

auto Button::draw(engine::Context context) -> void {
    Widget::draw(context);
}
