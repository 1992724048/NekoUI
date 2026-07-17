#pragma once
#include "../Widget.hpp"

namespace neko::widget {

struct RowStyle {
    type::Color background_color{0x00000000};
    type::Vec2 size{400, 50};
    float padding{8.0f};
    float spacing{4.0f};
};

/// 水平布局：子 Widget 沿 X 轴依次排列
class Row final : public Widget {
public:
    explicit Row(engine::Context&);

    auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
    auto hit_test(const device::Mouse& mouse) const -> bool override;

    auto style(const RowStyle& s) -> Row& {
        style_ = s;
        return *this;
    }

private:
    RowStyle style_{};
};

} // namespace neko::widget
