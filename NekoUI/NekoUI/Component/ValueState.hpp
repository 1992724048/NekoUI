#pragma once
#include <algorithm>
#include <atomic>
#include <functional>
#include <utility>

namespace neko::state {
    class ValueStateBase {
    protected:
        std::function<void()> mark_dirty;
    public:
        auto bind(const std::function<void()>& dirty) -> void {
            mark_dirty = dirty;
        }
    };

    template<typename T>
    class ValueState : public ValueStateBase {
        T value;
    public:
        auto operator()(std::function<void()> callback) -> void {
            this->mark_dirty = callback;
        }

        auto ref() -> T& {
            return value;
        }

        explicit operator T() {
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
