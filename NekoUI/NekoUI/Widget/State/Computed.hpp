#pragma once
#include "State.hpp"
#include <functional>
#include <utility>

namespace neko::state {

template<typename T>
class Computed {
    std::function<T()> compute_;
    State<T> cached_;
    bool dirty_ = true;
public:
    template<typename F>
    Computed(F&& fn) : compute_(std::forward<F>(fn)) {}

    auto get() -> const T& {
        if (dirty_) {
            cached_ = compute_();
            dirty_ = false;
        }
        return cached_.get();
    }
    operator const T&() { return get(); }

    // Declare dependency on one or more State<T>s
    template<typename... States>
    void depends_on(States&... states) {
        (states.set_observer([this](auto&) { dirty_ = true; }), ...);
    }
};

} // namespace neko::state
