#pragma once
#include "../Style/CSS.hpp"
#include <string_view>

namespace neko::widget {

/// CRTP mixin：提供链式 style/size/background/border setter，返回 Derived&
/// 不继承 Widget，使用多继承组合: class Button : public Widget, public Stylable<Button> {}
template <typename Derived>
class Stylable {
public:
    auto style(std::string_view name) -> Derived& {
        static_cast<Derived*>(this)->class_name_ = name;
        return static_cast<Derived&>(*this);
    }

    auto background(style::Background bg) -> Derived& {
        background_ = std::move(bg);
        return static_cast<Derived&>(*this);
    }

    auto widget_size(style::Size sz) -> Derived& {
        size_ = std::move(sz);
        return static_cast<Derived&>(*this);
    }

    auto border(style::Border bd) -> Derived& {
        border_ = std::move(bd);
        return static_cast<Derived&>(*this);
    }

protected:
    style::Background background_;
    style::Size size_{};
    style::Border border_;
};

} // namespace neko::widget
