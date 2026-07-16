#include "Button.hpp"

#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <utility>

using namespace neko::type;

namespace neko::widget {
    Button::Button(Widget* parent, const Vec4I bounds, const std::string_view label) :
        Widget{parent},
        text_{label.data(), label.size()} {
        this->bounds = bounds;
    }

    Button::Button(engine::Context& context, const Vec4I bounds, const std::string_view label) :
        Widget{context},
        text_{label.data(), label.size()} {
        this->bounds = bounds;
    }

    auto Button::set_text(const std::string_view text) -> void {
        {
            std::unique_lock lock(mutex_);
            text_.assign(text.data(), text.size());
        }
        if (context) {
            context->mark_dirty();
        }
    }

    auto Button::text() const -> std::string {
        std::shared_lock lock(mutex_);
        return text_;
    }

    auto Button::set_color(const Color color) -> void {
        {
            std::unique_lock lock(mutex_);
            color_ = color;
        }
        if (context) {
            context->mark_dirty();
        }
    }

    auto Button::color() const -> Color {
        std::shared_lock lock(mutex_);
        return color_;
    }

    auto Button::set_on_click(std::function<void()> callback) -> void {
        std::unique_lock lock(mutex_);
        on_click_ = std::move(callback);
    }

    auto Button::animate_color(const Color target, const int duration_ms) -> void {
        if (context) {
            color_anim_.bind(context->anim_inc, context->anim_dec);
        }
        color_anim_.to_value(static_cast<int>(target.value), std::optional{std::chrono::milliseconds{duration_ms}});
    }

    auto Button::draw_self(engine::Context& /*context*/, backend::Backend& backend) -> void {
        Vec4I b{};
        Color fill{};
        std::string label;
        {
            std::shared_lock lock(mutex_);
            b = bounds;
            fill = color_;
            label = text_;
        }

        backend.draw_rect_fill(b, fill);
        if (!label.empty()) {
            const Vec2I pos{b.x + 8, b.y + ((b.w - 16) / 2)};
            backend.draw_text(label, pos, Color{0xFF000000});
        }

        if (color_anim_.is_active()) {
            color_anim_();
        }
    }
} // namespace neko::widget
