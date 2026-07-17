#pragma once
#include "../Widget.hpp"

namespace neko::widget {

/// Column 样式
struct ColumnStyle {
    type::Color background_color{0x00000000};
    type::Vec2 size{400, 300};
    float padding{8.0f};
    float spacing{4.0f};
};

class Column final : public Widget {
public:
    explicit Column(engine::Context&);

    auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void override;
    auto build(engine::Context& context) -> void override;
    auto event(engine::Context& context) -> void override;
    auto input(engine::Context& context, const platform::Event& event) -> void override;
    [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;

    /// 链式 setter：设置样式
    auto style(ColumnStyle s) -> Column& {
        style_ = s;
        return *this;
    }

private:
    ColumnStyle style_{};
};

} // namespace neko::widget
