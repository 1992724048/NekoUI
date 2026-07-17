#include "Column.hpp"

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Platform/Event.hpp"

namespace neko::widget {
    auto Column::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void {
        // 1. 绘制背景
        if (background_.color.value != 0) {
            backend.draw_rect_fill(bounds, background_.color);
        }

        // 2. 绘制边框
        if (border_.size > 0.0f) {
            backend.draw_rect(bounds, border_.color, static_cast<int>(border_.size));
        }

        // 子 Widget 绘制由 WidgetTree 递归遍历处理，
        // Column 本身只负责自身的背景/边框绘制
    }

    auto Column::build(engine::Context& context) -> void {
        // 子 Widget 构建由 WidgetTree::traverse_impl 递归处理
    }

    auto Column::event(engine::Context& context) -> void {}

    auto Column::input(engine::Context& context, const platform::Event& event) -> void {
        // 输入事件由 EventRouter 通过 WidgetTree 的 hit_test 精准分发到目标 Widget，
        // 无需在容器层手动转发
    }

    auto Column::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }
} // namespace neko::widget
