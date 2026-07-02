#pragma once

#include <type_traits>

#include "Widget.hpp"

namespace neko::widget {
    template<typename T>
    class Sub : public T {
        static_assert(std::is_base_of_v<Widget, T>);
    public:
        template<typename... Args>
        explicit Sub(Widget* parent, Args&&... args) : T(std::forward<Args>(args)...) {
            if (parent) {
                parent->register_child(this);
            }
        }

        ~Sub() noexcept {
            if (this->m_parent) {
                Widget::unregister_child(this);
            }
        }

        Sub(const Sub&) = delete;
        auto operator=(const Sub&) -> Sub& = delete;
        Sub(Sub&&) = default;
        auto operator=(Sub&&) -> Sub& = default;
    };
} // namespace neko::widget
