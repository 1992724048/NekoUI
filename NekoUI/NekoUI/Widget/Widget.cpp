#include "Widget.hpp"

namespace neko::widget {
    auto Widget::draw(engine::Context& context, backend::Backend& backend) -> void {
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

    Widget::~Widget() = default;
}
