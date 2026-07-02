### Task 1: Extract standalone easing functions to Animation.hpp

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Component/Animation.hpp`

**Interfaces:**
- Produces: 30 standalone `auto easing_name(float t) -> float` functions in `neko::animation` namespace
- Keeps existing `out_bounce_impl` helper as-is

- [ ] **Step 1: Add 30 easing functions at end of Animation.hpp**

Add after line 835 (the line `} // namespace neko::animation`), before the closing brace.

In the `neko::animation` namespace, right before the closing `}`, add:

```cpp
// ── Standalone easing functions ──
inline auto linear(float t) -> float { return t; }

// Sine
inline auto ease_in_sine(float t) -> float { return 1.0F - std::cos(t * std::numbers::pi_v<float> * 0.5F); }
inline auto ease_out_sine(float t) -> float { return std::sin(t * std::numbers::pi_v<float> * 0.5F); }
inline auto ease_in_out_sine(float t) -> float { return -(std::cos(std::numbers::pi_v<float> * t) - 1.0F) * 0.5F; }

// Quad
inline auto ease_in_quad(float t) -> float { return t * t; }
inline auto ease_out_quad(float t) -> float { return 1.0F - (1.0F - t) * (1.0F - t); }
inline auto ease_in_out_quad(float t) -> float {
    return t < 0.5F ? 2.0F * t * t : 1.0F - (-2.0F * t + 2.0F) * (-2.0F * t + 2.0F) * 0.5F;
}

// Cubic
inline auto ease_in_cubic(float t) -> float { return t * t * t; }
inline auto ease_out_cubic(float t) -> float { return 1.0F - (1.0F - t) * (1.0F - t) * (1.0F - t); }
inline auto ease_in_out_cubic(float t) -> float {
    return t < 0.5F ? 4.0F * t * t * t : 1.0F - (-2.0F * t + 2.0F) * (-2.0F * t + 2.0F) * (-2.0F * t + 2.0F) * 0.5F;
}

// Quart
inline auto ease_in_quart(float t) -> float { auto v = t * t; return v * v; }
inline auto ease_out_quart(float t) -> float { auto u = 1.0F - t; auto v = u * u; return 1.0F - v * v; }
inline auto ease_in_out_quart(float t) -> float {
    if (t < 0.5F) { auto u = 2.0F * t; auto v = u * u; return v * v * 0.5F; }
    auto u = -2.0F * t + 2.0F; auto v = u * u; return (2.0F - v * v) * 0.5F;
}

// Quint
inline auto ease_in_quint(float t) -> float { auto v = t * t; return v * v * t; }
inline auto ease_out_quint(float t) -> float { auto u = 1.0F - t; auto v = u * u; return 1.0F - v * v * u; }
inline auto ease_in_out_quint(float t) -> float {
    if (t < 0.5F) { auto u = 2.0F * t; auto v = u * u; return v * v * u * 0.5F; }
    auto u = -2.0F * t + 2.0F; auto v = u * u; return (2.0F - v * v * u) * 0.5F;
}

// Expo
inline auto ease_in_expo(float t) -> float { return t == 0.0F ? 0.0F : std::pow(2.0F, 10.0F * t - 10.0F); }
inline auto ease_out_expo(float t) -> float { return t == 1.0F ? 1.0F : 1.0F - std::pow(2.0F, -10.0F * t); }
inline auto ease_in_out_expo(float t) -> float {
    if (t == 0.0F || t == 1.0F) return t;
    return t < 0.5F ? std::pow(2.0F, 20.0F * t - 10.0F) * 0.5F
                    : (2.0F - std::pow(2.0F, -20.0F * t + 10.0F)) * 0.5F;
}

// Circ
inline auto ease_in_circ(float t) -> float { return 1.0F - std::sqrt(1.0F - t * t); }
inline auto ease_out_circ(float t) -> float { return std::sqrt(1.0F - (t - 1.0F) * (t - 1.0F)); }
inline auto ease_in_out_circ(float t) -> float {
    return t < 0.5F ? (1.0F - std::sqrt(1.0F - 4.0F * t * t)) * 0.5F
                    : (std::sqrt(1.0F - (-2.0F * t + 2.0F) * (-2.0F * t + 2.0F)) + 1.0F) * 0.5F;
}

// Back
inline auto ease_in_back(float t) -> float { constexpr float c1 = 1.70158F; constexpr float c3 = c1 + 1.0F; return c3 * t * t * t - c1 * t * t; }
inline auto ease_out_back(float t) -> float { constexpr float c1 = 1.70158F; constexpr float c3 = c1 + 1.0F; auto u = t - 1.0F; return 1.0F + c3 * u * u * u + c1 * u * u; }
inline auto ease_in_out_back(float t) -> float {
    constexpr float c1 = 1.70158F; constexpr float c2 = c1 * 1.525F;
    return t < 0.5F ? (2.0F * t * 2.0F * t * ((c2 + 1.0F) * 2.0F * t - c2)) * 0.5F
                    : ((2.0F * t - 2.0F) * (2.0F * t - 2.0F) * ((c2 + 1.0F) * (t * 2.0F - 2.0F) + c2) + 2.0F) * 0.5F;
}

// Elastic
inline auto ease_in_elastic(float t) -> float {
    if (t == 0.0F || t == 1.0F) return t;
    constexpr float c4 = 2.0F * std::numbers::pi_v<float> / 3.0F;
    return -std::pow(2.0F, 10.0F * t - 10.0F) * std::sin((t * 10.0F - 10.75F) * c4);
}
inline auto ease_out_elastic(float t) -> float {
    if (t == 0.0F || t == 1.0F) return t;
    constexpr float c4 = 2.0F * std::numbers::pi_v<float> / 3.0F;
    return std::pow(2.0F, -10.0F * t) * std::sin((t * 10.0F - 0.75F) * c4) + 1.0F;
}
inline auto ease_in_out_elastic(float t) -> float {
    if (t == 0.0F || t == 1.0F) return t;
    constexpr float c5 = 2.0F * std::numbers::pi_v<float> / 4.5F;
    if (t < 0.5F) return -(std::pow(2.0F, 20.0F * t - 10.0F) * std::sin((20.0F * t - 11.125F) * c5)) * 0.5F;
    return (std::pow(2.0F, -20.0F * t + 10.0F) * std::sin((20.0F * t - 11.125F) * c5)) * 0.5F + 1.0F;
}

// Bounce
inline auto ease_in_bounce(float t) -> float { return 1.0F - out_bounce_impl(1.0F - t); }
inline auto ease_out_bounce(float t) -> float { return out_bounce_impl(t); }
inline auto ease_in_out_bounce(float t) -> float {
    return t < 0.5F ? (1.0F - out_bounce_impl(1.0F - 2.0F * t)) * 0.5F
                    : (1.0F + out_bounce_impl(2.0F * t - 1.0F)) * 0.5F;
}
```

- [ ] **Step 2: Verify** — Animation.hpp compiles standalone with `<cmath>`, `<numbers>` already included.

- [ ] **Step 3: Commit**
```bash
git add NekoUI/NekoUI/Widget/Component/Animation.hpp
git commit -m "feat: extract 30 standalone easing functions to Animation.hpp"
```
