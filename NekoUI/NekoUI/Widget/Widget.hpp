#pragma once
#include <memory>
#include <string>

#include "../Type.hpp"
#include "../Engine/Context.hpp"

namespace neko::widget {
    /**
     * <summary>
     * 控件对象
     * </summary>
     */
    class Widget {
    public:
        virtual ~Widget() = default;
        virtual auto draw(engine::Context context) -> void {}
    };
} // namespace neko::widget
