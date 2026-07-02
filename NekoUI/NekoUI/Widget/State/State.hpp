#pragma once
#include <functional>
#include <utility>
#include <vector>

namespace neko::state {

inline thread_local std::vector<std::function<void()>> g_auto_bind_stack;

template<typename T>
class State {
    T value_;
    std::function<void()> on_change_;
public:
    State() { try_auto_bind(); }
    State(T val) : value_(std::move(val)) { try_auto_bind(); }

    auto operator=(const T& new_val) -> State& {
        if (value_ != new_val) [[likely]] {
            value_ = new_val;
            if (on_change_) {
                on_change_();
            }
        }
        return *this;
    }

    auto get() const -> const T& {
        return value_;
    }

    operator const T&() const {
        return value_;
    }

    void set_on_change(std::function<void()> fn) {
        on_change_ = std::move(fn);
    }
private:
    void try_auto_bind() {
        if (!g_auto_bind_stack.empty()) {
            on_change_ = g_auto_bind_stack.back();
        }
    }
};

} // namespace neko::state
