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

namespace neko::engine {
    class WidgetTree;
}

namespace neko::widget {
    using namespace neko::type;

    class Widget {
    public:
        Widget() = default;
        virtual ~Widget();

        virtual auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void {}
        virtual auto build(engine::Context& context) -> void {}
        virtual auto event(engine::Context& context) -> void {}
        virtual auto input(engine::Context& context, const platform::Event& event) -> void {}

        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool {
            return false;
        }

        [[nodiscard]] auto id() const -> const std::string&;
        [[nodiscard]] auto index() const -> int;
        [[nodiscard]] auto path() const -> const std::string&;
    protected:
        Vec4I bounds{.width = std::numeric_limits<int>::max(), .height = std::numeric_limits<int>::max()};

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
    private:
        friend engine::WidgetTree;

        int z_index_{0};
        std::string id_;
        std::string path_;
    };
}
