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

namespace neko::engine {
    class WidgetTree;
}

namespace neko::widget {
    using namespace neko::type;

    class Widget {
    public:
        virtual ~Widget();

        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void = 0;
        virtual auto build(engine::Context& context) -> void = 0;
        virtual auto event(engine::Context& context) -> void = 0;

        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool = 0;

        [[nodiscard]] auto id() const -> const std::string&;
        [[nodiscard]] auto index() const -> int;
    protected:
        Vec4I bounds{.width = std::numeric_limits<int>::max(), .height = std::numeric_limits<int>::max()};

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
    private:
        friend engine::WidgetTree;

        int z_index_{0};
        std::string id_;
    };
}
