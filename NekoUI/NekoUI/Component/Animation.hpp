#pragma once
#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <numbers>
#include <optional>
#include <type_traits>

namespace neko::component {
    namespace ease {
        inline namespace linear {
            constexpr auto in(const float t) -> float {
                return t;
            }
        } // namespace linear

        namespace sine {
            constexpr auto in(const float t) -> float {
                return 1.0F - std::cos(t * std::numbers::pi_v<float> * 0.5F);
            }

            constexpr auto out(const float t) -> float {
                return std::sin(t * std::numbers::pi_v<float> * 0.5F);
            }

            constexpr auto in_out(const float t) -> float {
                return -(std::cos(std::numbers::pi_v<float> * t) - 1.0F) * 0.5F;
            }
        } // namespace sine

        namespace quad {
            constexpr auto in(const float t) -> float {
                return t * t;
            }

            constexpr auto out(const float t) -> float {
                const float u = 1.0F - t;
                return 1.0F - (u * u);
            }

            constexpr auto in_out(const float t) -> float {
                if (t < 0.5F) {
                    return 2.0F * t * t;
                }
                const float u = (-2.0F * t) + 2.0F;
                return 1.0F - (u * u * 0.5F);
            }
        } // namespace quad

        namespace cubic {
            constexpr auto in(const float t) -> float {
                return t * t * t;
            }

            constexpr auto out(const float t) -> float {
                const float u = 1.0F - t;
                return 1.0F - (u * u * u);
            }

            constexpr auto in_out(const float t) -> float {
                if (t < 0.5F) {
                    return 4.0F * t * t * t;
                }
                const float u = (-2.0F * t) + 2.0F;
                return 1.0F - (u * u * u * 0.5F);
            }
        } // namespace cubic

        namespace quart {
            constexpr auto in(const float t) -> float {
                const float v = t * t;
                return v * v;
            }

            constexpr auto out(const float t) -> float {
                const float u = 1.0F - t;
                const float v = u * u;
                return 1.0F - (v * v);
            }

            constexpr auto in_out(const float t) -> float {
                if (t < 0.5F) {
                    const float u = 2.0F * t;
                    const float v = u * u;
                    return v * v * 0.5F;
                }
                const float u = (-2.0F * t) + 2.0F;
                const float v = u * u;
                return (2.0F - (v * v)) * 0.5F;
            }
        } // namespace quart

        namespace quint {
            constexpr auto in(const float t) -> float {
                const float v = t * t;
                return v * v * t;
            }

            constexpr auto out(const float t) -> float {
                const float u = 1.0F - t;
                const float v = u * u;
                return 1.0F - (v * v * u);
            }

            constexpr auto in_out(const float t) -> float {
                if (t < 0.5F) {
                    const float u = 2.0F * t;
                    const float v = u * u;
                    return v * v * u * 0.5F;
                }
                const float u = (-2.0F * t) + 2.0F;
                const float v = u * u;
                return (2.0F - (v * v * u)) * 0.5F;
            }
        } // namespace quint

        namespace expo {
            constexpr auto in(const float t) -> float {
                if (t == 0.0F) {
                    return 0.0F;
                }
                return std::exp2((10.0F * t) - 10.0F);
            }

            constexpr auto out(const float t) -> float {
                if (t == 1.0F) {
                    return 1.0F;
                }
                return 1.0F - std::exp2(-10.0F * t);
            }

            constexpr auto in_out(const float t) -> float {
                if (t == 0.0F) {
                    return 0.0F;
                }
                if (t == 1.0F) {
                    return 1.0F;
                }
                if (t < 0.5F) {
                    return std::exp2((20.0F * t) - 10.0F) * 0.5F;
                }
                return (2.0F - std::exp2((-20.0F * t) + 10.0F)) * 0.5F;
            }
        } // namespace expo

        namespace circ {
            constexpr auto in(const float t) -> float {
                return 1.0F - std::sqrt(1.0F - (t * t));
            }

            constexpr auto out(const float t) -> float {
                const float u = t - 1.0F;
                return std::sqrt(1.0F - (u * u));
            }

            constexpr auto in_out(const float t) -> float {
                if (t < 0.5F) {
                    return (1.0F - std::sqrt(1.0F - (4.0F * t * t))) * 0.5F;
                }
                const float u = (-2.0F * t) + 2.0F;
                return (std::sqrt(1.0F - (u * u)) + 1.0F) * 0.5F;
            }
        } // namespace circ

        namespace back {
            constexpr auto in(const float t) -> float {
                constexpr float c1 = 1.70158F;
                constexpr float c3 = c1 + 1.0F;
                return (c3 * t * t * t) - (c1 * t * t);
            }

            constexpr auto out(const float t) -> float {
                constexpr float c1 = 1.70158F;
                constexpr float c3 = c1 + 1.0F;
                const float u = t - 1.0F;
                return 1.0F + (c3 * u * u * u) + (c1 * u * u);
            }

            constexpr auto in_out(const float t) -> float {
                constexpr float c1 = 1.70158F;
                constexpr float c2 = c1 * 1.525F;
                if (t < 0.5F) {
                    const float u = 2.0F * t;
                    const float u2 = u * u;
                    return 0.5F * u2 * (((c2 + 1.0F) * u) - c2);
                }
                const float u = (2.0F * t) - 2.0F;
                const float u2 = u * u;
                return 1.0F + (0.5F * u2 * (((c2 + 1.0F) * u) + c2));
            }
        } // namespace back

        namespace elastic {
            constexpr auto in(const float t) -> float {
                if (t == 0.0F || t == 1.0F) {
                    return t;
                }
                constexpr float c4 = 2.0F * std::numbers::pi_v<float> / 3.0F;
                const float exp_term = std::exp2((10.0F * t) - 10.0F);
                const float phase = ((t * 10.0F) - 10.75F) * c4;
                return -(exp_term * std::sin(phase));
            }

            constexpr auto out(const float t) -> float {
                if (t == 0.0F || t == 1.0F) {
                    return t;
                }
                constexpr float c4 = 2.0F * std::numbers::pi_v<float> / 3.0F;
                const float exp_term = std::exp2(-10.0F * t);
                const float phase = ((t * 10.0F) - 0.75F) * c4;
                return (exp_term * std::sin(phase)) + 1.0F;
            }

            constexpr auto in_out(const float t) -> float {
                if (t == 0.0F || t == 1.0F) {
                    return t;
                }
                constexpr float c5 = 2.0F * std::numbers::pi_v<float> / 4.5F;
                if (t < 0.5F) {
                    const float exp_term = std::exp2((20.0F * t) - 10.0F);
                    const float phase = ((20.0F * t) - 11.125F) * c5;
                    return -0.5F * exp_term * std::sin(phase);
                }
                const float exp_term = std::exp2((-20.0F * t) + 10.0F);
                const float phase = ((20.0F * t) - 11.125F) * c5;
                return (0.5F * exp_term * std::sin(phase)) + 1.0F;
            }
        } // namespace elastic

        namespace bounce {
            namespace detail {
                constexpr auto out_impl(float t) -> float {
                    constexpr float n1 = 7.5625F;
                    constexpr float d1 = 2.75F;
                    if (t < 1.0F / d1) {
                        return n1 * t * t;
                    }
                    if (t < 2.0F / d1) {
                        t -= 1.5F / d1;
                        return (n1 * t * t) + 0.75F;
                    }
                    if (t < 2.5F / d1) {
                        t -= 2.25F / d1;
                        return (n1 * t * t) + 0.9375F;
                    }
                    t -= 2.625F / d1;
                    return (n1 * t * t) + 0.984375F;
                }
            } // namespace detail

            constexpr auto in(const float t) -> float {
                return 1.0F - detail::out_impl(1.0F - t);
            }

            constexpr auto out(const float t) -> float {
                return detail::out_impl(t);
            }

            constexpr auto in_out(const float t) -> float {
                if (t < 0.5F) {
                    return (1.0F - detail::out_impl(1.0F - (2.0F * t))) * 0.5F;
                }
                return (1.0F + detail::out_impl((2.0F * t) - 1.0F)) * 0.5F;
            }
        } // namespace bounce
    } // namespace ease

    class AnimationBase {
    protected:
        std::function<void()> inc_{nullptr};
        std::function<void()> dec_{nullptr};
        std::atomic_bool change_{false};
    public:
        virtual ~AnimationBase() {
            if (change_.exchange(false) && dec_) {
                dec_();
            }
        }

        AnimationBase() = default;

        AnimationBase(const AnimationBase&) = delete;
        AnimationBase(AnimationBase&&) noexcept = delete;
        auto operator=(const AnimationBase&) -> AnimationBase& = delete;
        auto operator=(AnimationBase&&) noexcept -> AnimationBase& = delete;

        auto bind(const std::function<void()>& on_start, const std::function<void()>& on_end) -> void {
            inc_ = on_start;
            dec_ = on_end;
        }

        [[nodiscard]] auto is_active() const -> bool {
            return change_.load();
        }

        explicit operator bool() const {
            return is_active();
        }
    };

    template<typename Fn> concept EasingFn = std::is_invocable_r_v<float, Fn, float>;

    template<typename T, auto EasingFnType = ease::linear::in, typename TimeType = std::chrono::milliseconds> requires std::is_arithmetic_v<T> && EasingFn<decltype(EasingFnType)>
    class Animation final : public AnimationBase {
        T now_value_;
        T new_value_;
        T start_value_;
        TimeType duration_time_{};

        std::chrono::time_point<std::chrono::steady_clock> start_ = std::chrono::steady_clock::now();
    public:
        explicit Animation(T initial_value, const int duration_ms = 0) :
            now_value_{initial_value},
            new_value_{initial_value},
            start_value_{initial_value},
            duration_time_{std::chrono::duration_cast<TimeType>(std::chrono::milliseconds(duration_ms))} {}

        Animation(const Animation&) = delete;
        Animation(Animation&&) noexcept = delete;
        auto operator=(const Animation&) -> Animation& = delete;
        auto operator=(Animation&&) noexcept -> Animation& = delete;
        ~Animation() override = default;

        auto to_value(T target, std::optional<TimeType> custom_duration = std::nullopt) -> void {
            start_ = std::chrono::steady_clock::now();
            start_value_ = now_value_;
            new_value_ = target;
            if (custom_duration.has_value()) {
                duration_time_ = custom_duration.value();
            }
            if (!change_.exchange(true) && inc_) {
                inc_();
            }
        }

        auto tick() -> T {
            if (!change_) {
                return now_value_;
            }

            const auto elapsed = std::chrono::duration_cast<TimeType>(std::chrono::steady_clock::now() - start_);

            if (elapsed >= duration_time_) {
                now_value_ = new_value_;
                if (change_.exchange(false) && dec_) {
                    dec_();
                }
                return now_value_;
            }

            const float process = static_cast<float>(elapsed.count()) / static_cast<float>(duration_time_.count());
            const float eased = EasingFnType(process);
            const T interpolated = static_cast<T>(static_cast<float>(start_value_) + ((static_cast<float>(new_value_) - static_cast<float>(start_value_)) * eased));
            now_value_ = interpolated;
            return now_value_;
        }

        [[nodiscard]] auto value() const -> T {
            return now_value_;
        }

        auto operator()() -> T {
            return tick();
        }

        explicit operator T() {
            return tick();
        }

        auto operator=(T target) -> Animation& {
            to_value(target);
            return *this;
        }
    };
} // namespace neko::animation
