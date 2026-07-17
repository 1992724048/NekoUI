#pragma once
#include "../Widget.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {
    class Row final : public Widget {
    public:
        explicit Row(engine::Context&);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto background(style::Background bg) -> Row& {
            background_ = bg;
            return *this;
        }

        auto widget_size(style::Size sz) -> Row& {
            size_ = sz;
            return *this;
        }
    private:
        style::Background background_;
        style::Size size_{{400, 50}};
    };
} // namespace neko::widget
