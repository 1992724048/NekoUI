#pragma once
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "../Type.hpp"

#include "../Backend/Backend.hpp"

#include "../Engine/Context.hpp"

namespace neko::widget {
    /**
     * <summary>
     * 控件对象
     * </summary>
     */
    class Widget {
    protected:
        std::string key;
        std::vector<std::shared_ptr<Widget>> widgets;
    public:
        explicit Widget(const std::string& key = std::string("")) {
            if (key.empty()) {
                this->key = std::to_string(reinterpret_cast<std::uintptr_t>(this));
            } else {
                this->key = key;
            }
        }

        auto id() -> const std::string& {
            return key;
        }

        virtual ~Widget() = default;
        virtual auto draw(std::shared_ptr<backend::Backend>& backend) -> void {}
        virtual auto update(engine::Context& context) -> void {}

        [[nodiscard]] virtual auto children() const -> const std::vector<std::shared_ptr<Widget>>& {
            return widgets;
        }
    };
} // namespace neko::widget
