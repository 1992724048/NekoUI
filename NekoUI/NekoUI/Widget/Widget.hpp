// 2026-07-27 23:30:02

#pragma once

#include <algorithm>
#include <chrono>
#include <memory>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "../Type.hpp"

#include <concepts>
#include "../Engine/Context.hpp"
#include "../Engine/MutableWidget.hpp"
#include "../Engine/TreeManager.hpp"
#include "../Platform/Event.hpp"

#include "Behavior/LayoutBehavior.hpp"
#include "Behavior/DrawBehavior.hpp"
#include "Behavior/InputBehavior.hpp"
#include "Behavior/HitTestBehavior.hpp"

namespace neko::engine {
    class TreeManager;
}

namespace neko::backend {
    class DirectX11;
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

        template<typename B, typename... Args>
        auto add_behavior(Args&&... args) -> B& {
            static_assert(std::derived_from<B, Behavior>, "B 必须继承 Behavior");
            auto behavior = std::make_unique<B>(*this, std::forward<Args>(args)...);
            B& ref = *behavior;
            behaviors_.push_back(std::move(behavior));
            return ref;
        }

        virtual auto layout(Vec4I rect, engine::Context& context) -> void {
            for (const auto& behavior : behaviors_) {
                if (auto* layout = dynamic_cast<LayoutBehavior*>(behavior.get())) {
                    layout->layout(rect, context);
                    return;
                }
            }
        }

        virtual auto draw(Vec4I rect, engine::Context& context, backend::DirectX11& backend) -> Rect {
            for (const auto& behavior : behaviors_) {
                if (auto* draw = dynamic_cast<DrawBehavior*>(behavior.get())) {
                    return draw->draw(rect, context, backend);
                }
            }
            return {};
        }

        virtual auto input(engine::Context& context, const platform::Event& event) -> void {
            for (const auto& behavior : behaviors_) {
                if (auto* input = dynamic_cast<InputBehavior*>(behavior.get())) {
                    input->input(context, event);
                    return;
                }
            }
        }

        [[nodiscard]] virtual auto hit_test(const device::Mouse& mouse) const -> bool {
            for (const auto& behavior : behaviors_) {
                if (const auto* hit = dynamic_cast<const HitTestBehavior*>(behavior.get())) {
                    return hit->hit_test(mouse);
                }
            }
            return false;
        }

        [[nodiscard]] auto id() const -> const std::string&;
        [[nodiscard]] auto index() const -> int;
        [[nodiscard]] auto path() const -> const std::string&;

        [[nodiscard]] auto get_hovered() const -> bool {
            return hovered_.load(std::memory_order_relaxed);
        }

        auto set_hovered(const bool hovered) -> void {
            hovered_.store(hovered, std::memory_order_relaxed);
        }

        [[nodiscard]] auto get_bounds() const -> Vec4I {
            return bounds;
        }

        auto set_bounds(const Vec4I rect) -> void {
            bounds = rect;
        }

        [[nodiscard]] auto get_children() -> engine::MutableWidget& {
            return children_;
        }

        [[nodiscard]] auto get_children() const -> const engine::MutableWidget& {
            return children_;
        }
    protected:
        Vec4I bounds{.width = std::numeric_limits<int>::max(), .height = std::numeric_limits<int>::max()};

        Widget* parent_ = nullptr;
        std::weak_ptr<engine::Context> context_;

        std::atomic_bool isFocus{true};
        std::atomic_bool isDirty{true};
        std::atomic_bool hovered_{false};

        std::vector<std::unique_ptr<Behavior>> behaviors_;
    private:
        friend engine::TreeManager;

        engine::MutableWidget children_;

        int z_index_{0};
        std::string id_;
        std::string path_;
    };
}

namespace neko::widget {
    template<std::derived_from<Widget> T, typename... Args>
    auto Widget::build(Args&&... args) -> T& {
        // 子控件由 children_ 强持有，返回引用不悬垂
        const auto context = context_.lock();
        std::shared_ptr<T> child;
        if (context) {
            child = std::make_shared<T>(*context, std::forward<Args>(args)...);
        } else {
            // 生命周期契约破坏时的安全网：Context 已析构则用临时 Context 构造孤儿控件
            // （控件构造函数仅拷贝回调/忽略参数，不依赖 Context 存活；enable_shared_from_this
            // 的 protected 构造/析构要求 Context 由 shared_ptr 持有，不能栈上构造）
            const auto orphan_context = std::make_shared<engine::Context>();
            child = std::make_shared<T>(*orphan_context, std::forward<Args>(args)...);
        }
        child->parent_ = this;
        auto& ref = *child;

        // 树结构突变与渲染线程 layout/draw 遍历互斥（TreeManager::mutex_）
        std::unique_lock<std::shared_mutex> tree_lock;
        if (context) {
            if (const auto tree = context->tree_manager.lock()) {
                tree_lock = std::unique_lock<std::shared_mutex>(tree->mutex_);
            }
        }

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

        if (context && context->widget_tree_changed) {
            context->widget_tree_changed();
        }

        return ref;
    }

    template<std::invocable<Widget&> F>
    auto Widget::children(F&& fn) -> Widget& {
        fn(*this);
        return *this;
    }
} // namespace neko::widget
