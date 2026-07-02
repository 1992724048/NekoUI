#pragma once
#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "State.hpp"
#include "../../Engine/Context.hpp"
#include "../Component/Animation.hpp"

namespace neko::state {
    template<typename T>
    auto lerp(const T& a, const T& b, float t) -> T {
        return a + (b - a) * t;
    }

    inline auto lerp(const glm::vec4& a, const glm::vec4& b, const float t) -> glm::vec4 {
        return glm::mix(a, b, t);
    }

    inline auto lerp(const glm::ivec4& a, const glm::ivec4& b, const float t) -> glm::ivec4 {
        return glm::ivec4(glm::mix(glm::vec4(a), glm::vec4(b), t));
    }

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
        AnimatedState(T init, TimeType dur, const EasingFn ease = animation::ease_out_quad) : current_(init),
                                                                                              target_(init),
                                                                                              start_(init),
                                                                                              duration_(dur),
                                                                                              easing_(ease) {}

        auto set_easing(const EasingFn fn) -> void {
            easing_ = fn;
        }

        auto operator=(const T& new_target) -> AnimatedState& {
            if (target_ != new_target) {
                start_ = current_.get();
                target_ = new_target;
                elapsed_ = TimeType::zero();
                animating_ = true;
                if (ctx_) {
                    ctx_->animation_start();
                }
            }
            return *this;
        }

        auto update(TimeType dt) -> void {
            if (!animating_) {
                return;
            }
            elapsed_ += dt;
            const float t = std::min(1.0f, static_cast<float>(elapsed_.count()) / static_cast<float>(duration_.count()));
            float eased = easing_(t);
            current_ = lerp(start_, target_, eased);
            if (t >= 1.0f) {
                current_ = target_;
                animating_ = false;
                if (ctx_) {
                    ctx_->animation_end();
                }
            }
        }

        void set_on_change(std::function<void()> fn) {
            current_.set_on_change(std::move(fn));
        }

        auto get() const -> const T& {
            return current_.get();
        }

        operator const T&() const {
            return get();
        }

        [[nodiscard]] auto animating() const -> bool {
            return animating_;
        }

        auto set_context(engine::Context& ctx) -> void {
            ctx_ = &ctx;
        }
    };
} // namespace neko::state
