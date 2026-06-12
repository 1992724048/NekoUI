// 2026-06-02 20:38:26

#pragma once
#include <chrono>
#include <numbers>

namespace neko::animation {
    //! @brief 动画基类
    //! @tparam T 值类型
    //! @tparam TimeType 时间类型 
    //! @note https://easings.net/
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class Animation {
    protected:
        TimeType duration;
        T now_value;
        T new_value;
        T start_value;

        std::chrono::time_point<std::chrono::steady_clock> start;

        bool change{false};
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

        //! @brief 更新并获取当前变换值
        //! @return 值
        virtual auto update() -> T& {
            change = false;
            return now_value = new_value;
        }

        //! @brief 值
        operator T() {
            return update();
        }

        //! @brief 设置变动值
        //! @param value 值
        //! @param duration 时间
        auto to_value(T value, std::optional<TimeType> duration = std::nullopt) -> void {
            start = std::chrono::high_resolution_clock::now();
            start_value = now_value;
            new_value = value;
            if (duration.has_value()) {
                this->duration = duration.value();
            }
            change = true;
        }

        //! @brief 设置变动值
        //! @param value 值
        //! @param duration 时间
        auto operator()(T value, std::optional<TimeType> duration = std::nullopt) -> void {
            return to_value(value, duration);
        }

        //! @brief 获取进度
        //! @return 百分比
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

        //! @brief 是否完成
        //! @return true: 完成动画 \n false: 播放动画
        [[nodiscard]] auto is_done() const -> bool {
            return !change;
        }
    };

    //! @brief 线性动画
    //! @tparam T 值类型
    //! @tparam TimeType 时间类型
    //! @note https://easings.net/
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class LinearAnimation final : public Animation<T, TimeType> {
    public:
        explicit LinearAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        //! @brief 更新并获取当前变换值
        //! @return 值
        auto update() -> T& override {
            if (this->change) {
                float process = this->progress();
                this->now_value = this->start_value + ((this->new_value - this->start_value) * process);
            }
            return this->now_value;
        }
    };

    //! @brief EaseInOut
    //! @tparam T 值类型
    //! @tparam TimeType 时间类型
    //! @note https://easings.net/
    template<typename T, typename TimeType = std::chrono::milliseconds>
    class EaseInOutSineAnimation final : public Animation<T, TimeType> {
    public:
        explicit EaseInOutSineAnimation(T value, int duration = 0) : Animation<T, TimeType>(value, duration) {}

        //! @brief 更新并获取当前变换值
        //! @return 值
        auto update() -> T& override {
            if (this->change) {
                const float process = this->progress();
                const float eased = (1.0F - std::cos(std::numbers::pi_v<float> * process)) * 0.5F;
                this->now_value = this->start_value + ((this->new_value - this->start_value) * eased);
            }
            return this->now_value;
        }
    };
} // namespace neko::animation
