#pragma once

#include <algorithm>
#include <chrono>
#include <climits>
#include <shared_mutex>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Type.hpp"

#include "../Backend/Backend.hpp"
#include "../Engine/Context.hpp"
#include "../Platform/Event.hpp"
#include "../Engine/MutableWidget.hpp"
#include <concepts>

namespace neko::engine {
    class WidgetTree;
}

namespace neko::widget {
    using namespace neko::type;

    class Widget {
    public:
        Widget() = default;
        virtual ~Widget();

        // ── Builder API ──
        /// 在 this 下创建 T 类型子 Widget，返回 T& 用于链式配置
        template<std::derived_from<Widget> T, typename... Args>
        auto build(Args&&... args) -> T&;

        /// 子节点作用域：fn 接收 *this（父 Widget 引用）
        template<std::invocable<Widget&> F>
        auto children(F&& fn) -> Widget&;

        /// 获取父 Widget（根节点返回 nullptr）
        [[nodiscard]] auto parent() const -> Widget*;

        virtual auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void {}
        virtual auto build(engine::Context& context) -> void {}
        virtual auto event(engine::Context& context) -> void {}
        virtual auto input(engine::Context& context, const platform::Event& event) -> void {}

        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool {
            return false;
        }

        [[nodiscard]] auto id() const -> const std::string&;
        [[nodiscard]] auto index() const -> int;
        [[nodiscard]] auto path() const -> const std::string&;
    protected:
        Vec4I bounds{.width = std::numeric_limits<int>::max(), .height = std::numeric_limits<int>::max()};

        Widget* parent_ = nullptr;

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
    private:
        friend engine::WidgetTree;

        engine::MutableWidget children_{};

        int z_index_{0};
        std::string id_;
        std::string path_;

        [[nodiscard]] auto get_children() -> engine::MutableWidget& { return children_; }
        [[nodiscard]] auto get_children() const -> const engine::MutableWidget& { return children_; }
    };
}

namespace neko::widget {

// ── Builder API 模板实现 ──

template<std::derived_from<Widget> T, typename... Args>
auto Widget::build(Args&&... args) -> T& {
    auto child = std::make_shared<T>(std::forward<Args>(args)...);
    child->parent_ = this;
    auto& ref = *child;

    // 自动转换：monostate → shared_ptr → list
    std::visit(
        [&]<typename V>(V& val) {
            if constexpr (std::is_same_v<V, std::monostate>) {
                children_ = engine::MutableWidget(std::move(child));
            } else if constexpr (std::is_same_v<V, std::shared_ptr<Widget>>) {
                engine::internal::MutableWidgetList list;
                list.emplace_back(engine::MutableWidget(std::move(val)));
                list.emplace_back(engine::MutableWidget(std::move(child)));
                children_ = engine::MutableWidget(std::move(list));
            } else if constexpr (std::is_same_v<V, engine::internal::MutableWidgetList>) {
                val.emplace_back(engine::MutableWidget(std::move(child)));
            } else if constexpr (std::is_same_v<V, engine::internal::MutableWidgetVector>) {
                val.emplace_back(engine::MutableWidget(std::move(child)));
            }
        },
        static_cast<engine::internal::WidgetContainer&>(children_));

    return ref;
}

template<std::invocable<Widget&> F>
auto Widget::children(F&& fn) -> Widget& {
    fn(*this);
    return *this;
}

inline auto Widget::parent() const -> Widget* {
    return parent_;
}

}  // namespace neko::widget
