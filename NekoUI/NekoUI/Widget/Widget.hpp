#pragma once

#include "../Backend/Backend.hpp"

namespace neko::widget {
    class Widget {
    public:
        virtual ~Widget() = default;
        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void {}
        virtual auto layout(glm::vec4 constraints) -> void {};
        virtual auto update(engine::Context& context) -> void {}

        [[nodiscard]] virtual auto child_count() const -> size_t {
            return 0;
        }

        virtual auto dirty() -> bool {
            return false;
        }

        virtual auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool {
            return false;
        }
    };
} // namespace neko::widget
