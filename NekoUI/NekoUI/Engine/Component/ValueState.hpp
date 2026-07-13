#pragma once
#include <algorithm>
#include <functional>
#include <utility>

namespace neko::state {
    template<typename T>
    class ValueState {
        T value;
        std::function<void()> mark_dirty;
    public:
        auto operator()(std::function<void()> mark_dirty) -> void {
            this->mark_dirty = std::move(mark_dirty);
        }

        auto ref() -> T& {
            return value;
        }

        operator T() {
            return ref();
        }

        auto operator=(T& v) -> T& {
            mark_dirty();
            return value = v;
        }

        auto operator=(T&& v) -> T& {
            mark_dirty();
            return value = v;
        }
    };
} // namespace neko::state
