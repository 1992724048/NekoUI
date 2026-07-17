#include "Widget.hpp"

namespace neko::widget {
    Widget::~Widget() = default;

    auto Widget::id() const -> const std::string& {
        return id_;
    }

    auto Widget::index() const -> int {
        return z_index_;
    }

    auto Widget::path() const -> const std::string& {
        return path_;
    }

    auto Widget::get_bounds() const -> const Vec4I& {
        return bounds;
    }

    auto Widget::set_bounds(const Vec4I b) -> void {
        bounds = b;
    }

    auto Widget::parent() const -> Widget* {
        return parent_;
    }
}
