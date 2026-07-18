#pragma once
#include "../Widget.hpp"
#include "../../Style/Stylable.hpp"

namespace neko::widget {
    class Row final : public Widget, public Stylable<Row> {
    public:
        explicit Row(engine::Context&);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
} // namespace neko::widget
