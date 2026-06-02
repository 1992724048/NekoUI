#pragma once
#include <memory>
#include <string>

namespace neko::widget {
    /**
     * <summary>
     * 控件对象
     * </summary>
     */
    class Widget {
    public:
        virtual ~Widget() = default;
        std::string id_key;
        std::atomic<std::shared_ptr<Widget>> child;

        virtual auto draw() -> void {};
    };
} // namespace neko::widget
