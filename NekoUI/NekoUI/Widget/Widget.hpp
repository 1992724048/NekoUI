#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <vector>

#include <glm/glm.hpp>

#include "../Backend/Backend.hpp"
#include "../Engine/Context.hpp"

namespace neko::widget {
    struct Constraints {
        int x = 0, y = 0;
        int width = INT_MAX;
        int height = INT_MAX;
    };

    class Widget {
        friend class engine::Engine;
    public:
        virtual auto draw(std::unique_ptr<engine::Context>& context, std::unique_ptr<backend::Backend>& backend) -> void {}
        virtual auto layout(Constraints constraints) -> void {}
        virtual auto update(std::unique_ptr<engine::Context>& context) -> void {}
        virtual auto raw_event(std::unique_ptr<engine::Context>& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool;

        template<typename T, typename... Args> requires std::is_base_of_v<Widget, T>
        auto add(Args&&... args) -> std::shared_ptr<T> {
            return std::make_shared<T>(this, std::forward<Args>(args)...);
        }

        [[nodiscard]] virtual auto hit_test(const mouse::Mouse& mouse) const -> bool;
    protected:
        ~Widget();
        explicit Widget(engine::Engine* engine);
        explicit Widget(const std::weak_ptr<Widget>& parent);

        engine::Engine* engine;
    private:
        std::atomic<std::weak_ptr<Widget>> parent{};
        std::atomic<std::weak_ptr<Widget>> root{};

        glm::ivec4 bounds{};

        int z_index{0};

        std::atomic_bool isVisible{true};
        std::atomic_bool isFocus{true};
    };
} // namespace neko::widget
