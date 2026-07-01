#pragma once

#include "../Widget.hpp"

namespace neko::widget::layout {

    // === VStack: 垂直排列子控件 ===
    class VStack : public Widget {
    public:
        explicit VStack(int spacing = 4) : m_spacing(spacing) {}

        auto layout(Constraints constraints) -> void override {
            int current_y = constraints.y;
            for (auto* child : children()) {
                child->set_bounds(constraints.x, current_y,
                                  constraints.width, child->height());
                current_y += child->height() + m_spacing;
                child->layout({constraints.x, 0, constraints.width, child->height()});
            }
        }

    private:
        int m_spacing = 4;
    };

    // === HStack: 水平排列子控件 ===
    class HStack : public Widget {
    public:
        explicit HStack(int spacing = 4) : m_spacing(spacing) {}

        auto layout(Constraints constraints) -> void override {
            int current_x = constraints.x;
            for (auto* child : children()) {
                child->set_bounds(current_x, constraints.y,
                                  child->width(), constraints.height);
                current_x += child->width() + m_spacing;
                child->layout({0, constraints.y, child->width(), constraints.height});
            }
        }

    private:
        int m_spacing = 4;
    };

    // === ZStack: 层叠排列子控件（重叠，按 z_order 绘制） ===
    class ZStack : public Widget {
    public:
        auto layout(Constraints constraints) -> void override {
            for (auto* child : children()) {
                child->set_bounds(constraints.x, constraints.y,
                                  constraints.width, constraints.height);
                child->layout(constraints);
            }
        }
    };

} // namespace neko::widget::layout
