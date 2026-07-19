#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"

namespace neko::widget {
    class Row final : public Widget,
                      public style::BackgroundStyle,
                      public style::SizeStyle {
    public:
        explicit Row(engine::Context&);

        auto layout(Vec4I rect, engine::Context& context) -> void override;
        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
} // namespace neko::widget
