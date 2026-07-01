#pragma once
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "../Type.hpp"

#include "../Backend/Backend.hpp"

#include "../Engine/Engine.hpp"

namespace neko::widget {
    //! @brief 控件对象
    class Widget {
    protected:
        std::string key;
        std::vector<std::shared_ptr<Widget>> widgets{};
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

        virtual auto dirty() -> bool {
            return false;
        }

        virtual ~Widget() = default;
        virtual auto draw(engine::Context& context, backend::Backend& backend) -> void {}
        virtual auto update(engine::Context& context) -> void {}

        //! @brief 获取子控件
        //! @return 控件数组
        //! @note 循序决定层次, 请将最底层控件放置在数组底部
        [[nodiscard]] virtual auto children() const -> const std::vector<std::shared_ptr<Widget>>& {
            return widgets;
        }
    };
} // namespace neko::widget
