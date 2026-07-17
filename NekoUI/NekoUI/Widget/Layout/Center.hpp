#pragma once
#include "../Widget.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {

    class Center final : public Widget {
    public:
        explicit Center(engine::Context&);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto background(style::Background bg) -> Center& { background_ = bg; return *this; }

    private:
        style::Background background_;
    };

} // namespace neko::widget
