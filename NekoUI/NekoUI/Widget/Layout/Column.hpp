#pragma once
#include "../Widget.hpp"
#include "../Stylable.hpp"

namespace neko::widget {

    class Column final : public Widget, public Stylable<Column> {
    public:
        explicit Column(engine::Context&);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };

} // namespace neko::widget
