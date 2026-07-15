#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <vector>

#include "../Type.hpp"

#include "../Backend/Backend.hpp"
#include "../Engine/Context.hpp"

namespace neko::widget {
    using namespace neko::type;

    struct Constraints {
        int x = 0, y = 0;
        int width = INT_MAX;
        int height = INT_MAX;
    };

    class Widget {
        friend class engine::Engine;
    public:
        explicit Widget(engine::Engine* engine);
        explicit Widget(Widget* parent);

        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void {}
        virtual auto layout(Constraints constraints) -> void {}
        virtual auto update(engine::Context& context) -> void {}
        virtual auto raw_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool;

        template<typename T, typename... Args> requires std::is_base_of_v<Widget, T>
        auto add(Args&&... args) -> std::shared_ptr<T> {
            return std::make_shared<T>(engine, std::forward<Args>(args)...);
        }

        [[nodiscard]] virtual auto hit_test(const mouse::Mouse& mouse) const -> bool;
    protected:
        ~Widget();

        std::atomic<Widget*> parent{};
        std::atomic<std::weak_ptr<Widget>> root{};

        Vec4I bounds{};

        int z_index{0};

        std::atomic_bool isVisible{true};
        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};

        std::string id;
    private:
        engine::Engine* engine;
    };
} // namespace neko::widget
