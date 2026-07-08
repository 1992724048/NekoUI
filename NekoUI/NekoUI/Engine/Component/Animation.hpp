// 2026-07-01 23:43:09

#pragma once
#include <chrono>
#include <cmath>
#include <numbers>

namespace neko::animation {
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class Animation {
    protected:
        TimeType duration_time;
        T now_value;
        T new_value;
        T start_value;

        std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::high_resolution_clock::now();

        std::atomic_bool change{false};
    public:
        explicit Animation(T value, int duration = 0) {
            now_value = value;
            new_value = value;
            start_value = value;
            this->duration_time = TimeType(duration);
            start = std::chrono::steady_clock::now();
        }

        Animation(const Animation& other) : duration_time(other.duration_time),
                                            now_value(other.now_value),
                                            new_value(other.new_value),
                                            start_value(other.start_value),
                                            start(other.start),
                                            change(other.change.load()) {}

        Animation(Animation&& other) noexcept : duration_time(std::move(other.duration_time)),
                                                now_value(std::move(other.now_value)),
                                                new_value(std::move(other.new_value)),
                                                start_value(std::move(other.start_value)),
                                                start(std::move(other.start)),
                                                change(other.change.load()) {}

        auto operator=(const Animation& other) -> Animation& {
            if (this != &other) {
                duration_time = other.duration_time;
                now_value = other.now_value;
                new_value = other.new_value;
                start_value = other.start_value;
                start = other.start;
                change.store(other.change.load());
            }
            return *this;
        }

        auto operator=(Animation&& other) noexcept -> Animation& {
            if (this != &other) {
                duration_time = std::move(other.duration_time);
                now_value = std::move(other.now_value);
                new_value = std::move(other.new_value);
                start_value = std::move(other.start_value);
                start = std::move(other.start);
                change.store(other.change.load());
            }
            return *this;
        }
    protected:
        virtual ~Animation() = default;
    public:
        virtual auto update() -> T& {
            change = false;
            return now_value = new_value;
        }

        explicit operator T() {
            return update();
        }

        auto to_value(T value, std::optional<TimeType> duration = std::nullopt) -> void {
            start = std::chrono::steady_clock::now();
            start_value = now_value;
            new_value = value;
            if (duration.has_value()) {
                this->duration_time = duration.value();
            }
            change = true;
        }

        auto operator()() -> T& {
            return update();
        }

        auto operator=(T value) -> Animation& {
            to_value(value);
            return *this;
        }

        auto progress() -> float {
            auto elapsed = std::chrono::duration_cast<TimeType>(std::chrono::steady_clock::now() - start);
            if (elapsed >= duration_time) {
                now_value = new_value;
                start_value = new_value;
                if (this->change) {
                    this->change = false;
                }
                return 1.0F;
            }

            return static_cast<float>(elapsed.count()) / static_cast<float>(duration_time.count());
        }

        [[nodiscard]] auto is_done() const -> bool {
            return !change;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class LinearAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit LinearAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                float process = this->progress();
                this->now_value = this->start_value + ((this->new_value - this->start_value) * process);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInSineAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInSineAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = 1.0F - std::cos(process * std::numbers::pi_v<float> * 0.5F);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutSineAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutSineAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = std::sin(process * std::numbers::pi_v<float> * 0.5F);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutSineAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutSineAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = -(std::cos(std::numbers::pi_v<float> * process) - 1.0F) * 0.5F;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInQuadAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInQuadAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process * process;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutQuadAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutQuadAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float u = 1.0F - process;
                const float eased = 1.0F - (u * u);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutQuadAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutQuadAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process < 0.5F ? 2.0F * process * process : 1.0F - (((-2.0F * process) + 2.0F) * ((-2.0F * process) + 2.0F) * 0.5F);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInCubicAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInCubicAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process * process * process;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutCubicAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutCubicAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float u = 1.0F - process;
                const float eased = 1.0F - (u * u * u);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutCubicAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutCubicAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process < 0.5F ? 4.0F * process * process * process : 1.0F - (((-2.0F * process) + 2.0F) * ((-2.0F * process) + 2.0F) * ((-2.0F * process) + 2.0F) * 0.5F);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInQuartAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInQuartAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float v = process * process;
                const float eased = v * v;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutQuartAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutQuartAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float u = 1.0F - process;
                const float v = u * u;
                const float eased = 1.0F - (v * v);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutQuartAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutQuartAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased;
                if (process < 0.5F) {
                    const float u = 2.0F * process;
                    const float v = u * u;
                    eased = v * v * 0.5F;
                } else {
                    const float u = (-2.0F * process) + 2.0F;
                    const float v = u * u;
                    eased = (2.0F - (v * v)) * 0.5F;
                }
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInQuintAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInQuintAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float v = process * process;
                const float eased = v * v * process;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutQuintAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutQuintAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float u = 1.0F - process;
                const float v = u * u;
                const float eased = 1.0F - (v * v * u);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutQuintAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutQuintAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased;
                if (process < 0.5F) {
                    const float u = 2.0F * process;
                    const float v = u * u;
                    eased = v * v * u * 0.5F;
                } else {
                    const float u = (-2.0F * process) + 2.0F;
                    const float v = u * u;
                    eased = (2.0F - (v * v * u)) * 0.5F;
                }
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInExpoAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInExpoAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process == 0.0F ? 0.0F : std::pow(2.0F, (10.0F * process) - 10.0F);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutExpoAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutExpoAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process == 1.0F ? 1.0F : 1.0F - std::pow(2.0F, -10.0F * process);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutExpoAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutExpoAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased;
                if (process == 0.0F) {
                    eased = 0.0F;
                } else if (process == 1.0F) {
                    eased = 1.0F;
                } else if (process < 0.5F) {
                    eased = std::pow(2.0F, (20.0F * process) - 10.0F) * 0.5F;
                } else {
                    eased = (2.0F - std::pow(2.0F, (-20.0F * process) + 10.0F)) * 0.5F;
                }
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInCircAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInCircAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = 1.0F - std::sqrt(1.0F - (process * process));
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutCircAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutCircAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = std::sqrt(1.0F - ((process - 1.0F) * (process - 1.0F)));
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutCircAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutCircAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process < 0.5F ?
                                        (1.0F - std::sqrt(1.0F - (4.0F * process * process))) * 0.5F :
                                        (std::sqrt(1.0F - (((-2.0F * process) + 2.0F) * ((-2.0F * process) + 2.0F))) + 1.0F) * 0.5F;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInBackAnimation final : public Animation<T, TimeType> {
        static constexpr float c1 = 1.70158F;
        static constexpr float c3 = c1 + 1.0F;
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInBackAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = (c3 * process * process * process) - (c1 * process * process);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutBackAnimation final : public Animation<T, TimeType> {
        static constexpr float c1 = 1.70158F;
        static constexpr float c3 = c1 + 1.0F;
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutBackAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float u = process - 1.0F;
                const float eased = 1.0F + (c3 * u * u * u) + (c1 * u * u);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutBackAnimation final : public Animation<T, TimeType> {
        static constexpr float c1 = 1.70158F;
        static constexpr float c2 = c1 * 1.525F;
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutBackAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process < 0.5F ?
                                        2.0F * process * (2.0F * process) * (((c2 + 1.0F) * 2.0F * process) - c2) * 0.5F :
                                        ((((2.0F * process) - 2.0F) * ((2.0F * process) - 2.0F) * (((c2 + 1.0F) * ((process * 2.0F) - 2.0F)) + c2)) + 2.0F) * 0.5F;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInElasticAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInElasticAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased;
                if (process == 0.0F || process == 1.0F) {
                    eased = process;
                } else {
                    constexpr float c4 = 2.0F * std::numbers::pi_v<float> / 3.0F;
                    eased = -std::pow(2.0F, (10.0F * process) - 10.0F) * std::sin(((process * 10.0F) - 10.75F) * c4);
                }
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutElasticAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutElasticAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased;
                if (process == 0.0F || process == 1.0F) {
                    eased = process;
                } else {
                    constexpr float c4 = 2.0F * std::numbers::pi_v<float> / 3.0F;
                    eased = (std::pow(2.0F, -10.0F * process) * std::sin(((process * 10.0F) - 0.75F) * c4)) + 1.0F;
                }
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutElasticAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutElasticAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased;
                if (process == 0.0F || process == 1.0F) {
                    eased = process;
                } else {
                    constexpr float c5 = 2.0F * std::numbers::pi_v<float> / 4.5F;
                    if (process < 0.5F) {
                        eased = -(std::pow(2.0F, (20.0F * process) - 10.0F) * std::sin(((20.0F * process) - 11.125F) * c5)) * 0.5F;
                    } else {
                        eased = (std::pow(2.0F, (-20.0F * process) + 10.0F) * std::sin(((20.0F * process) - 11.125F) * c5) * 0.5F) + 1.0F;
                    }
                }
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    inline auto out_bounce_impl(float t) -> float {
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

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInBounceAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInBounceAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = 1.0F - out_bounce_impl(1.0F - process);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseOutBounceAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseOutBounceAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = out_bounce_impl(process);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };

    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutBounceAnimation final : public Animation<T, TimeType> {
    public:
        using Animation<T, TimeType>::operator=;

        explicit EaseInOutBounceAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = process < 0.5F ? (1.0F - out_bounce_impl(1.0F - (2.0F * process))) * 0.5F : (1.0F + out_bounce_impl((2.0F * process) - 1.0F)) * 0.5F;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };
} // namespace neko::animation
