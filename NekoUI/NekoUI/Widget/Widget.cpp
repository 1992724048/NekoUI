#include "Widget.hpp"

namespace neko::widget {
    Widget::~Widget() = default;

    auto Widget::id() const -> const std::string& {
        return id_;
    }

    auto Widget::index() const -> int {
        return z_index_;
    }
}
