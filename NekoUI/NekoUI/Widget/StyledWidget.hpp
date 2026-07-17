#pragma once

#include "Widget.hpp"
#include "../Style/CSS.hpp"

namespace neko::widget {

/// CRTP 中间层：setter 返回 Derived& 以保持链式调用的派生类型
template <typename Derived>
class StyledWidget : public Widget {
public:
    auto set_background(style::Background bg) -> Derived& {
        background_ = std::move(bg);
        return static_cast<Derived&>(*this);
    }

    auto set_size(style::Size sz) -> Derived& {
        size_ = std::move(sz);
        return static_cast<Derived&>(*this);
    }

    auto set_border(style::Border bd) -> Derived& {
        border_ = std::move(bd);
        return static_cast<Derived&>(*this);
    }

protected:
    style::Background background_{};
    style::Size size_{};
    style::Border border_{};
};

}  // namespace neko::widget
