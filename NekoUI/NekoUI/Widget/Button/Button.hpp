#pragma once
#include "../Widget.hpp"

#include <functional>
#include <string>
#include <string_view>

#include "../../Component/Animation.hpp"

namespace neko::widget {
    class Button final : public Widget {
    public:
        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
} // namespace neko::widget
