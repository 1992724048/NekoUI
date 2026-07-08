#pragma once
#include <algorithm>

namespace neko::state {
    template<typename T>
    class ValueState {
        T value;
    public:
        auto ref() -> T& {
            return value;
        }

        operator T() {
            return ref();
        }

        auto operator=(T& v) -> T& {
            return value = v;
        }

        auto operator=(T&& v) -> T& {
            return value = v;
        }
    };
} // namespace neko::state
