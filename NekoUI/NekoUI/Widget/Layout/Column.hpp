#pragma once
#include "../Widget.hpp"

#include "../../Style/CSS.hpp"

namespace neko::widget {

    class Column final : public Widget {
    public:
        explicit Column(engine::Context& /*unused*/);

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

        auto background(style::Background bg) -> Column& { background_ = bg; return *this; }
        auto widget_size(style::Size sz) -> Column& { size_ = sz; return *this; }

    private:
        style::Background background_;
        style::Size size_{{400, 300}};
    };

} // namespace neko::widget
