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
        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void {}
        virtual auto layout(Constraints constraints) -> void {}
        virtual auto raw_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool;

        [[nodiscard]] virtual auto hit_test(const mouse::Mouse& mouse) const -> bool;

        [[nodiscard]] auto id() const -> const std::string&;
    protected:
        ~Widget();

        std::atomic<Widget*> parent{};
        std::atomic<std::weak_ptr<Widget>> root{};

        Vec4I bounds{};

        int z_index{0};

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
    private:
        std::string id_;
    };
}
