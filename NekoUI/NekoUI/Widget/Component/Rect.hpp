#pragma once
#include "../Widget.hpp"

namespace neko::widget {
    //! @brief 矩形控件（填充矩形）
    class RectWidget final : public Widget {
    public:
        RectWidget(const glm::ivec4 rect, const type::Color color) : rect_(rect),
                                                                     color_(color) {}

        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            backend.draw_rect_fill(rect_, color_);
        }
    private:
        glm::ivec4 rect_;
        type::Color color_;
    };
} // namespace neko::widget
