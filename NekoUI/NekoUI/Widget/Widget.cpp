#include "Widget.hpp"

#include "../Engine/Engine.hpp"

neko::widget::Widget::~Widget() {
    engine->del(this);
}

auto neko::widget::Widget::raw_event(std::unique_ptr<engine::Context>& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool {
    return false;
}

auto neko::widget::Widget::hit_test(const mouse::Mouse& mouse) const -> bool {
    return false;
}

neko::widget::Widget::Widget(engine::Engine* engine) {
    this->engine = engine;
    this->root = engine->root.load();
}

neko::widget::Widget::Widget(const std::weak_ptr<Widget>& parent) {
    if (!parent.expired()) {
        return;
    }

    const auto ptr = parent.lock();

    this->engine = ptr->engine;
    this->root = ptr->root.load();
    this->parent = parent;
}
