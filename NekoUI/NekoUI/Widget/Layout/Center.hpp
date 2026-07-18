#pragma once
#include "../Widget.hpp"
#include "../../Style/CSS.hpp"

namespace neko::widget {
    class Center final : public Widget,
                         public style::BackgroundStyle {
    public:
        explicit Center(engine::Context& /*unused*/);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };

} // namespace neko::widget
