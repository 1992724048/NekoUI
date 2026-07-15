#include "Widget.hpp"

#include "../Engine/Engine.hpp"

neko::widget::Widget::~Widget() {
    engine->del_widget(this);
}

auto neko::widget::Widget::raw_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool {
    return false;
}

auto neko::widget::Widget::hit_test(const mouse::Mouse& mouse) const -> bool {
    return false;
}

neko::widget::Widget::Widget(engine::Engine* engine) {
    this->engine = engine;
    this->root = engine->root.load();
}

neko::widget::Widget::Widget(Widget* parent) {
    this->engine = parent->engine;
    this->root = parent->root.load();
    this->parent = parent;
}
