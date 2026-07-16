#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <shared_mutex>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Type.hpp"

#include "../Backend/Backend.hpp"
#include "../Engine/Context.hpp"
#include "../Platform/Event.hpp"

#include <numbers>

namespace neko::widget {
    using namespace neko::type;

    struct Constraints {
        int x = 0, y = 0;
        int width = std::numeric_limits<int>::max();
        int height = std::numeric_limits<int>::max();
    };

    class Widget {
    public:
        auto draw(engine::Context& context, backend::Backend& backend) -> void;
        auto create(engine::Context& context) -> void;
        auto update(engine::Context& context) -> void;
        auto layout(Constraints constraints) -> void;

        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool;

        template<typename T, typename... Args> requires std::is_base_of_v<Widget, T>
        auto add(Args&&... args) -> std::shared_ptr<T> {
            auto child = std::make_shared<T>(this, std::forward<Args>(args)...);
            {
                std::unique_lock lock(mutex_);
                children_.push_back(child);
            }
            if (context) {
                context->reg_widget(child);
            }
            return child;
        }

        auto set_id(std::string_view id) -> void;
        [[nodiscard]] auto children_count() const -> size_t;

        [[nodiscard]] auto id() const -> const std::string&;
    protected:
        engine::Context* context{};

        ~Widget();

        mutable std::shared_mutex mutex_{};
        std::vector<std::shared_ptr<Widget>> children_{};

        Vec4I bounds{};

        int z_index{0};

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
    private:
        std::string id_;
    };
}
