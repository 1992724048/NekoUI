// 2026-08-10

#include "TextFieldHitTest.hpp"

#include "../../Device/Mouse.hpp"
#include "../Widget.hpp"

namespace neko::behavior {
    auto TextFieldHitTest::hit_test(const device::Mouse& mouse) const -> bool {
        return mouse.is_inside(geometry_.bounds);
    }
} // namespace neko::behavior
