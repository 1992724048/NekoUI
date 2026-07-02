#pragma once
#include <functional>
#include <utility>

namespace neko::state {

template<typename T>
class State {
    T value_;
    std::function<void(const T&)> observer_;
public:
    State() = default;
    State(T val) : value_(std::move(val)) {}

    auto operator=(const T& new_val) -> State& {
        if (value_ != new_val) [[likely]] {
            value_ = new_val;
            if (observer_) observer_(value_);
        }
        return *this;
    }

    auto get() const -> const T& { return value_; }
    operator const T&() const { return value_; }

    void set_observer(std::function<void(const T&)> obs) { observer_ = std::move(obs); }
};

} // namespace neko::state
