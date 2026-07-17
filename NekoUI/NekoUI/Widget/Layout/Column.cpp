#include "Column.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"

namespace neko::widget {
    Column::Column(engine::Context&) {}

    auto Column::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void {
        // 1. 绘制背景
        if (style_.background_color.value != 0) {
            backend.draw_rect_fill(bounds, style_.background_color);
        }

        // 子 Widget 绘制由 WidgetTree 递归遍历处理，
        // Column 本身只负责自身的背景/边框绘制
    }

    auto Column::build(engine::Context& context) -> void {
        // 子 Widget 构建由 WidgetTree::build_recursive 处理
    }

    auto Column::event(engine::Context& context) -> void {}

    auto Column::input(engine::Context& context, const platform::Event& event) -> void {
        // 输入事件由 EventRouter 通过 WidgetTree 分发
    }

    auto Column::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
