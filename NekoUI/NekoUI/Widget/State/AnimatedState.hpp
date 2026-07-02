#pragma once
#include "State.hpp"
#include "../../Engine/Context.hpp"
#include "../Component/Animation.hpp"
#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace neko::state {

// ── lerp (linear interpolation) ──
template<typename T>
auto lerp(const T& a, const T& b, float t) -> T {
    return a + (b - a) * t;
}

inline auto lerp(const glm::vec4& a, const glm::vec4& b, float t) -> glm::vec4 {
    return glm::mix(a, b, t);
}

inline auto lerp(const glm::ivec4& a, const glm::ivec4& b, float t) -> glm::ivec4 {
    return glm::ivec4(glm::mix(glm::vec4(a), glm::vec4(b), t));
}

// ── AnimatedState ──
template<typename T, typename TimeType = std::chrono::milliseconds>
class AnimatedState {
    using EasingFn = float(*)(float);

    State<T> current_;
    T target_, start_;
    TimeType duration_;
    TimeType elapsed_{0};
    EasingFn easing_ = animation::ease_out_quad;
    bool animating_ = false;
    engine::Context* ctx_ = nullptr;

public:
    AnimatedState(T init, TimeType dur, EasingFn ease = animation::ease_out_quad)
        : current_(init), target_(init), start_(init), duration_(dur), easing_(ease) {}

    auto set_easing(EasingFn fn) -> void { easing_ = fn; }

    auto operator=(const T& new_target) -> AnimatedState& {
        if (target_ != new_target) {
            start_ = current_.get();
            target_ = new_target;
            elapsed_ = TimeType::zero();
            animating_ = true;
            if (ctx_) ctx_->animation_start();
        }
        return *this;
    }

    void update(TimeType dt) {
        if (!animating_) return;
        elapsed_ += dt;
        float t = std::min(1.0f, static_cast<float>(elapsed_.count())
                                 / static_cast<float>(duration_.count()));
        float eased = easing_(t);
        current_ = lerp(start_, target_, eased);
        if (t >= 1.0f) {
            current_ = target_;
            animating_ = false;
            if (ctx_) ctx_->animation_end();
        }
    }

    void set_observer(std::function<void(const T&)> obs) { current_.set_observer(std::move(obs)); }
    auto get() const -> const T& { return current_.get(); }
    operator const T&() const { return get(); }
    [[nodiscard]] auto animating() const -> bool { return animating_; }
    void set_context(engine::Context& ctx) { ctx_ = &ctx; }
};

} // namespace neko::state
