#pragma once
#include "../StyledWidget.hpp"

namespace neko::widget {
    /// 垂直布局：子 Widget 沿 Y 轴依次排列
    class Column final : public StyledWidget<Column> {
    public:
        Column() = default;

        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void override;
        auto build(engine::Context& context) -> void override;
        auto event(engine::Context& context) -> void override;
        auto input(engine::Context& context, const platform::Event& event) -> void override;
        [[nodiscard]] auto hit_test(const device::Mouse& mouse) const -> bool override;
    };
} // namespace neko::widget
