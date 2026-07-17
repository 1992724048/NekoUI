#include "Button.hpp"

#include <limits>
#include <utility>

#include "../../Backend/Backend.hpp"
#include "../../Device/Mouse.hpp"
#include "../../Platform/Event.hpp"

namespace neko::widget {
    Button::Button(engine::Context&, std::string text, std::function<void()> on_click) :
        text_(std::move(text)),
        on_click_(std::move(on_click)) {}

    auto Button::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect {
        // 1. 从全局样式表查找，合并到本地属性
        if (const auto* rule = context.stylesheet.get(class_name_)) {
            if (rule->background) {
                background_ = *rule->background;
            }
            if (rule->size) {
                size_ = *rule->size;
            }
            if (rule->border) {
                border_ = *rule->border;
            }
            if (rule->font_size) {
                font_size_ = *rule->font_size;
            }
            if (rule->text_color) {
                text_color_ = *rule->text_color;
            }
        }

        // 2. 无样式表规则时使用 ColorScheme 默认值
        auto bg = background_.color.value != 0 ? background_ : style::Background{context.scheme.primary};
        auto tc = text_color_.value != 0 ? text_color_ : Color{0xFFFFFFFF};

        // 3. 用 rect 参数计算有效区域（不修改 bounds）
        auto effective = rect;
        const auto use_parent = size_.size.x == std::numeric_limits<float>::max() || size_.size.y == std::numeric_limits<float>::max();
        if (!use_parent) {
            effective.z = effective.x + static_cast<int>(size_.size.x);
            effective.w = effective.y + static_cast<int>(size_.size.y);
        }

        backend.draw_rect_fill(effective, bg.color);

        if (border_.size > 0.0F) {
            backend.draw_rect(effective, border_.color, static_cast<int>(border_.size));
        }

        if (!text_.empty()) {
            const auto text_pos = Vec2I{.x = static_cast<int>(effective.x + static_cast<float>(effective.z) * 0.1F), .y = static_cast<int>(effective.y + static_cast<float>(effective.w) * 0.5F)};
            backend.draw_text(text_, text_pos, tc, font_size_);
        }

        return Rect{.x = effective.x, .y = effective.y, .width = effective.z - effective.x, .height = effective.w - effective.y};
    }

    auto Button::build(engine::Context& context) -> void {}

    auto Button::event(engine::Context& context) -> void {}

    auto Button::input(engine::Context& context, const platform::Event& event) -> void {
        const auto* mouse_evt = std::get_if<device::MouseButtonEvent>(&event);
        if (mouse_evt && mouse_evt->button == device::MouseButton::Left && mouse_evt->pressed) {
            if (on_click_ && context.mouse.lock()) {
                if (context.mouse.lock()->is_inside(bounds)) {
                    on_click_();
                }
            }
        }
    }

    auto Button::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(bounds);
    }

    auto Button::on_click(std::function<void()> cb) -> Button& {
        on_click_ = std::move(cb);
        return *this;
    }

    auto Button::text(std::string t) -> Button& {
        text_ = std::move(t);
        return *this;
    }
} // namespace neko::widget
