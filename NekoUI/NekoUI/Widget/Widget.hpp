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

#include <concepts>
#include "../Backend/Backend.hpp"
#include "../Engine/Context.hpp"
#include "../Engine/MutableWidget.hpp"
#include "../Platform/Event.hpp"

namespace neko::engine {
    class WidgetTree;
}

namespace neko::widget {
    using namespace neko::type;

    class Widget {
    public:
        Widget() = default;
        virtual ~Widget();

        template<std::derived_from<Widget> T, typename... Args>
        auto build(Args&&... args) -> T&;

        template<std::invocable<Widget&> F>
        auto children(F&& fn) -> Widget&;

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
        engine::Context* context_ = nullptr;

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
    private:
        friend engine::WidgetTree;

        engine::MutableWidget children_{};

        int z_index_{0};
        std::string id_;
        std::string path_;

        [[nodiscard]] auto get_children() -> engine::MutableWidget& {
            return children_;
        }

        [[nodiscard]] auto get_children() const -> const engine::MutableWidget& {
            return children_;
        }
    };
}

namespace neko::widget {
    template<std::derived_from<Widget> T, typename... Args>
    auto Widget::build(Args&&... args) -> T& {
        auto child = std::make_shared<T>(*context_, std::forward<Args>(args)...);
        child->parent_ = this;
        auto& ref = *child;

        std::visit([&]<typename V>(V& val) -> auto {
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

        if (context_ && context_->on_widget_tree_changed) {
            context_->on_widget_tree_changed();
        }

        return ref;
    }

    template<std::invocable<Widget&> F>
    auto Widget::children(F&& fn) -> Widget& {
        fn(*this);
        return *this;
    }
} // namespace neko::widget
