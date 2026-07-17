#pragma once
#include "../Widget.hpp"

namespace neko::widget {
    struct ColumnStyle {
        Color background_color{};
        Vec2 size{.x = 400, .y = 300};
        float padding{8.0F};
        float spacing{4.0F};
    };

    class Column final : public Widget {
    public:
        explicit Column(engine::Context&);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto style(const ColumnStyle& s) -> Column&;
    private:
        ColumnStyle style_{};
    };
} // namespace neko::widget
