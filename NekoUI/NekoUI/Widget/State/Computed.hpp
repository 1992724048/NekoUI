#pragma once
#include <functional>
#include <utility>
#include "State.hpp"

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

        operator const T&() {
            return get();
        }

        template<typename... States>
        auto depends_on(States&... states) -> void {
            (states.set_observer([this](auto&) {
                dirty_ = true;
            }), ...);
        }
    };
} // namespace neko::state
