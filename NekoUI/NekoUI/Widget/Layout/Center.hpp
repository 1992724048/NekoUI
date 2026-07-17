#pragma once
#include "../Widget.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {
    struct CenterStyle {
        Color background_color{0x00000000};
    };

    class Center final : public Widget {
    public:
        explicit Center(engine::Context&);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto style(const CenterStyle& s) -> Center&;
    private:
        CenterStyle style_{};
    };
} // namespace neko::widget
