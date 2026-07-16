#include "Button.hpp"

#include <chrono>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <utility>

using namespace neko::type;

namespace neko::widget {
    auto Button::draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> void {}
    auto Button::build(engine::Context& context) -> void {}
    auto Button::event(engine::Context& context) -> void {}
    auto Button::input(engine::Context& context, const platform::Event& event) -> void {}

    auto Button::hit_test(const device::Mouse& mouse) const -> bool {
        return false;
    }
} // namespace neko::widget
