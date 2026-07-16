#include "Widget.hpp"

namespace neko::widget {
    auto Widget::draw(engine::Context& context, backend::Backend& backend) -> void {
        draw_self(context, backend);
        std::shared_lock lock(mutex_);
        for (const auto& child : children_) {
            child->draw(context, backend);
        }
    }

    auto Widget::layout(Constraints constraints) -> void {
        {
            std::unique_lock lock(mutex_);
            bounds = Vec4I{constraints.x, constraints.y, constraints.width, constraints.height};
        }
        std::shared_lock lock(mutex_);
        for (const auto& child : children_) {
            child->layout(Constraints{.x = constraints.x, .y = constraints.y, .width = constraints.width, .height = constraints.height});
        }
    }

    auto Widget::raw_event(engine::Context& context, const platform::Event& event) -> bool {
        std::shared_lock lock(mutex_);
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->raw_event(context, event)) {
                return true;
            }
        }
        return false;
    }

    auto Widget::hit_test(const device::Mouse& mouse) const -> bool {
        std::shared_lock lock(mutex_);
        if (mouse.is_inside(bounds)) {
            return true;
        }
        for (const auto& child : children_) {
            if (child->hit_test(mouse)) {
                return true;
            }
        }
        return false;
    }

    auto Widget::set_id(const std::string_view id) -> void {
        std::unique_lock lock(mutex_);
        id_ = id;
    }

    auto Widget::children_count() const -> size_t {
        std::shared_lock lock(mutex_);
        return children_.size();
    }

    auto Widget::id() const -> const std::string& {
        std::shared_lock lock(mutex_);
        return id_;
    }

    Widget::Widget(engine::Context& context) :
        context{&context},
        root{context.root} {}

    Widget::Widget(Widget* parent) :
        context{parent ? parent->context : nullptr},
        parent{parent},
        root{parent ? parent->root.load() : std::weak_ptr<Widget>{}} {}

    Widget::~Widget() = default;
}
