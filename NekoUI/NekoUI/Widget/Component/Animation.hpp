// 2026-06-02 20:38:26

#pragma once
#include <chrono>

namespace neko::animation {
    /**
     * <summary>
     * 动画基类
     * </summary>
     */
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class Animation {
    protected:
        T now_value;
        T new_value;
        T start_value;
        bool change{false};
        TimeType duration;

        std::chrono::time_point<std::chrono::steady_clock> start{};
    public:
        explicit Animation(T value, int duration = 0) {
            now_value = value;
            new_value = value;
            start_value = value;
            this->duration = TimeType(duration);
            start = std::chrono::high_resolution_clock::now();
        }

        Animation(const Animation& other) = default;
        Animation(Animation&& other) noexcept = default;
        auto operator=(const Animation& other) -> Animation& = default;
        auto operator=(Animation&& other) noexcept -> Animation& = default;

        virtual ~Animation() = default;

        /**
         * <summary>
         * 更新并获取当前变换值
         * </summary>
         * <returns>值</returns>
         */
        virtual auto update() -> T& {
            change = false;
            return now_value = new_value;
        }

        /**
         * <summary>
         * 设置变动值
         * </summary>
         * <param name="value">值</param>
         */
        auto to_value(T value) -> void {
            start = std::chrono::high_resolution_clock::now();
            start_value = now_value;
            new_value = value;
            change = true;
        }

        /**
         * <summary>
         * 获取进度
         * </summary>
         * <returns>结果</returns>
         */
        auto progress() -> float {
            const auto now = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::duration_cast<TimeType>(now - start);

            if (elapsed >= duration) {
                this->change = false;
                now_value = new_value;
                start_value = new_value;
                return 1.0F;
            }

            return static_cast<float>(elapsed.count()) / static_cast<float>(duration.count());
        }

        /**
         * <summary>
         * 是否完成
         * </summary>
         * <returns>结果</returns>
         */
        [[nodiscard]] auto is_done() const -> bool {
            return change;
        }
    };

    /**
     * <summary>
     * 线性动画
     * </summary>
     */
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class LinearAnimation final : public Animation<T, TimeType> {
    public:
        explicit LinearAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        /**
         * <summary>
         * 更新并获取当前变换值
         * </summary>
         * <returns>值</returns>
         */
        auto update() -> T& override {
            if (this->change) {
                float process = this->progress();
                this->now_value = this->start_value + ((this->new_value - this->start_value) * process);
            }
            return this->now_value;
        }
    };

    /**
     * <summary>
     * EaseInOut
     * </summary>
     */
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutAnimation final : public Animation<T, TimeType> {
    public:
        explicit EaseInOutAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        /**
         * <summary>
         * 更新并获取当前变换值
         * </summary>
         * <returns>值</returns>
         */
        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                float eased = process < 0.5F ? 4.0F * process * process * process : 1.0F - (std::pow((-2.0F * process) + 2.0F, 3.0F) / 2.0F);
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };
} // namespace neko::animation
