#pragma once
#include <string_view>
#include "../Style/CSS.hpp"

namespace neko::widget {
    template<typename Derived>
    class Stylable {
        Stylable() = default;
    public:
        auto style(std::string_view name) -> Derived& {
            static_cast<Derived*>(this)->class_name_ = name;
            return static_cast<Derived&>(*this);
        }
    protected:
        friend Derived;
    };
} // namespace neko::widget
