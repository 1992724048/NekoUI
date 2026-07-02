# Reactive State System — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add reactive State<T>, Computed<T>, AnimatedState<T> to NekoUI and migrate existing widgets.

**Architecture:** Signal-based reactivity. State<T> holds a value + observer callback. Widget::bind_state() wires observer to mark_dirty(). AnimatedState<T> drives interpolation on render thread via new Widget::animate(dt) virtual. 30 standalone easing functions extracted from existing Animation subclasses.

**Tech Stack:** C++26, GLM, Win32, DX11

## Global Constraints

- Namespace: `neko::state` for new types; `neko::animation` for easing functions
- Trailing return types (`auto foo() -> void`)
- `std::println` for console output
- Colors: `type::Color` = `glm::ivec4` (RGBA 0–255)
- Backend rects: `glm::ivec4(x, y, width, height)` — not x1/y1/x2/y2
- Only `Widget/State/` for new files; modify existing files in-place
- Add new .hpp files to NekoUI.vcxproj under `<ClInclude>` in alphabetical order
- GLM via `#include <glm/glm.hpp>`, stb_truetype vendored
- No third-party UI frameworks

---

### Task 1: Extract standalone easing functions to Animation.hpp

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Component/Animation.hpp`

**Interfaces:**
- Produces: 30 standalone `auto easing_name(float t) -> float` functions in `neko::animation`
- Produces: `neko::animation::out_bounce_impl` already exists, keep as-is

- [ ] **Step 1: Add 30 easing functions at end of Animation.hpp**

Add after line 835 (end of namespace `neko::animation`), before closing brace:

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

- [ ] **Step 2: Verify no placeholder code**

Animation.hpp compiles standalone with `<cmath>`, `<numbers>` already included.

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/Widget/Component/Animation.hpp
git commit -m "feat: extract 30 standalone easing functions to Animation.hpp"
```

---

### Task 2: Create `State<T>` header

**Files:**
- Create: `NekoUI/NekoUI/Widget/State/State.hpp`

**Interfaces:**
- Produces: `neko::state::State<T>` — see spec section 3

- [ ] **Step 1: Create Widget/State/State.hpp**

```cpp
#pragma once
#include <functional>
#include <utility>

namespace neko::state {

template<typename T>
class State {
    T value_;
    std::function<void(const T&)> observer_;
public:
    State() = default;
    State(T val) : value_(std::move(val)) {}

    auto operator=(const T& new_val) -> State& {
        if (value_ != new_val) [[likely]] {
            value_ = new_val;
            if (observer_) observer_(value_);
        }
        return *this;
    }

    auto get() const -> const T& { return value_; }
    operator const T&() const { return value_; }

    void set_observer(std::function<void(const T&)> obs) { observer_ = std::move(obs); }
};

} // namespace neko::state
```

- [ ] **Step 2: Commit**

```bash
git add NekoUI/NekoUI/Widget/State/State.hpp
git commit -m "feat: add State<T> reactive primitive"
```

---

### Task 3: Create `Computed<T>` header

**Files:**
- Create: `NekoUI/NekoUI/Widget/State/Computed.hpp`

**Interfaces:**
- Produces: `neko::state::Computed<T>` — see spec section 5

- [ ] **Step 1: Create Widget/State/Computed.hpp**

```cpp
#pragma once
#include "State.hpp"
#include <functional>
#include <utility>

namespace neko::state {

template<typename T>
class Computed {
    std::function<T()> compute_;
    State<T> cached_;
    bool dirty_ = true;
public:
    template<typename F>
    Computed(F&& fn) : compute_(std::forward<F>(fn)) {}

    auto get() -> const T& {
        if (dirty_) {
            cached_ = compute_();
            dirty_ = false;
        }
        return cached_.get();
    }
    operator const T&() { return get(); }

    // Declare dependency on one or more State<T>s
    template<typename... States>
    void depends_on(States&... states) {
        (states.set_observer([this](auto&) { dirty_ = true; }), ...);
    }
};

} // namespace neko::state
```

- [ ] **Step 2: Commit**

```bash
git add NekoUI/NekoUI/Widget/State/Computed.hpp
git commit -m "feat: add Computed<T> derived state"
```

---

### Task 4: Create `AnimatedState<T>` header with lerp

**Files:**
- Create: `NekoUI/NekoUI/Widget/State/AnimatedState.hpp`

**Interfaces:**
- Produces: `neko::state::AnimatedState<T>` — see spec section 7
- Produces: `neko::state::lerp()` specializations for arithmetic + glm types

- [ ] **Step 1: Create Widget/State/AnimatedState.hpp**

```cpp
#pragma once
#include "State.hpp"
#include "../../Engine/Context.hpp"
#include "../Component/Animation.hpp"
#include <algorithm>
#include <chrono>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace neko::state {

// ── lerp (linear interpolation) ──
template<typename T>
auto lerp(const T& a, const T& b, float t) -> T {
    return a + (b - a) * t;
}

inline auto lerp(const glm::vec4& a, const glm::vec4& b, float t) -> glm::vec4 {
    return glm::mix(a, b, t);
}

inline auto lerp(const glm::ivec4& a, const glm::ivec4& b, float t) -> glm::ivec4 {
    return glm::ivec4(glm::mix(glm::vec4(a), glm::vec4(b), t));
}

// ── AnimatedState ──
template<typename T, typename TimeType = std::chrono::milliseconds>
class AnimatedState {
    using EasingFn = float(*)(float);

    State<T> current_;
    T target_, start_;
    TimeType duration_;
    TimeType elapsed_{0};
    EasingFn easing_ = animation::ease_out_quad;
    bool animating_ = false;
    engine::Context* ctx_ = nullptr;

public:
    AnimatedState(T init, TimeType dur, EasingFn ease = animation::ease_out_quad)
        : current_(init), target_(init), start_(init), duration_(dur), easing_(ease) {}

    auto set_easing(EasingFn fn) -> void { easing_ = fn; }

    auto operator=(const T& new_target) -> AnimatedState& {
        if (target_ != new_target) {
            start_ = current_.get();
            target_ = new_target;
            elapsed_ = TimeType::zero();
            animating_ = true;
            if (ctx_) ctx_->animation_start();
        }
        return *this;
    }

    void update(TimeType dt) {
        if (!animating_) return;
        elapsed_ += dt;
        float t = std::min(1.0f, static_cast<float>(elapsed_.count())
                                 / static_cast<float>(duration_.count()));
        float eased = easing_(t);
        current_ = lerp(start_, target_, eased);
        if (t >= 1.0f) {
            current_ = target_;
            animating_ = false;
            if (ctx_) ctx_->animation_end();
        }
    }

    void set_observer(std::function<void(const T&)> obs) { current_.set_observer(std::move(obs)); }
    auto get() const -> const T& { return current_.get(); }
    operator const T&() const { return get(); }
    [[nodiscard]] auto animating() const -> bool { return animating_; }
    void set_context(engine::Context& ctx) { ctx_ = &ctx; }
};

} // namespace neko::state
```

- [ ] **Step 2: Commit**

```bash
git add NekoUI/NekoUI/Widget/State/AnimatedState.hpp
git commit -m "feat: add AnimatedState<T> with lerp specializations"
```

---

### Task 5: Integrate with Widget base class

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Widget.hpp` — add `bind_state()`, `animate(dt)` virtual
- Modify: `NekoUI/NekoUI/Widget/Widget.cpp` — add default `animate()` implementation

**Interfaces:**
- Produces: `Widget::bind_state(State<T>&)` — wires observer to `mark_dirty()`
- Produces: `Widget::animate(std::chrono::milliseconds dt)` — virtual, propagates to children
- Consumes: `neko::state::State<T>`, `neko::state::AnimatedState<T>`

- [ ] **Step 1: Add bind_state() and animate() to Widget.hpp**

Add include:
```cpp
#include "State/State.hpp"
```

Add in the protected section (after `Widget();`):
```cpp
        template<typename T>
        auto bind_state(State<T>& state) -> void;

        virtual auto animate(std::chrono::milliseconds dt) -> void;
```

- [ ] **Step 2: Implement animate() in Widget.cpp**

Add before the Widget constructor:
```cpp
auto neko::widget::Widget::animate(const std::chrono::milliseconds dt) -> void {
    for (auto* child : m_children) {
        if (child->m_visible) child->animate(dt);
    }
}
```

Add bind_state implementation (inline in header as template, or in .cpp). Since it's a template, keep in header:
```cpp
        template<typename T>
        auto bind_state(State<T>& state) -> void {
            state.set_observer([this](const T&) { mark_dirty(); });
        }
```

Put this in the header after the protected section.

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/Widget/Widget.hpp NekoUI/NekoUI/Widget/Widget.cpp
git commit -m "feat: add bind_state() and animate(dt) to Widget base"
```

---

### Task 6: Add animate step to Engine render loop

**Files:**
- Modify: `NekoUI/NekoUI/Engine/Engine.hpp` — add `render_frame(dt)` with TimeType param
- Modify: `NekoUI/NekoUI/Engine/Engine.cpp` — add animate step in render_frame, add dt tracking

**Interfaces:**
- Consumes: `Widget::animate(dt)` called on render thread before layout+draw

- [ ] **Step 1: Modify Engine.hpp**

No structural changes needed. The render_frame() method already exists. We need to add frame timing.

Add include for chrono:
```cpp
#include <chrono>
```

No other header changes needed — the `std::chrono::milliseconds` is already available.

- [ ] **Step 2: Modify Engine.cpp — add frame_dt tracking + animate step**

In `Engine.hpp`, add a member for tracking frame time. After `std::atomic_int animation{};`:
```cpp
        using Clock = std::chrono::steady_clock;
        Clock::time_point m_last_frame{Clock::now()};
```

In `Engine.cpp`, update `render_frame()`:
```cpp
auto Engine::render_frame() -> void {
    // Calculate delta time
    const auto now = Clock::now();
    const auto dt = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_last_frame);
    m_last_frame = now;

    if (resize_pending.exchange(false)) {
        backend.resize(resize_size);
    }

    backend.begin();
    for (const auto& root : m_root_widgets) {
        root->animate(dt);  // NEW: advance animations before layout+draw
        root->layout({0, 0, resize_size.x, resize_size.y});
        root->draw(context, backend);
    }
    backend.end();
    context.dirty = false;
}
```

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/Engine/Engine.hpp NekoUI/NekoUI/Engine/Engine.cpp
git commit -m "feat: add animate() step to render loop with frame delta time"
```

---

### Task 7: Add new State/* files to vcxproj

**Files:**
- Modify: `NekoUI/NekoUI.vcxproj`

- [ ] **Step 1: Add the three State headers to vcxproj**

Add after line 215 (`<ClInclude Include="NekoUI\Widget\Widget.hpp" />`):
```xml
    <ClInclude Include="NekoUI\Widget\State\AnimatedState.hpp" />
    <ClInclude Include="NekoUI\Widget\State\Computed.hpp" />
    <ClInclude Include="NekoUI\Widget\State\State.hpp" />
```

(Ordered alphabetically with the other Widget includes — they go right before Widget.hpp.)

Actually, they should go in the general `<ClInclude>` list in alphabetical order. Let me find the exact insertion point.

Add right before `NekoUI\Widget\Widget.hpp`:
```xml
    <ClInclude Include="NekoUI\Widget\State\AnimatedState.hpp" />
    <ClInclude Include="NekoUI\Widget\State\Computed.hpp" />
    <ClInclude Include="NekoUI\Widget\State\State.hpp" />
```

- [ ] **Step 2: Commit**

```bash
git add NekoUI/NekoUI.vcxproj
git commit -m "chore: add State/* headers to vcxproj"
```

---

### Task 8: Migrate Button to reactive state + AnimatedState

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Button/Button.hpp`
- Modify: `NekoUI/NekoUI/Widget/Button/Button.cpp`

**Interfaces:**
- Consumes: `AnimatedState<T>`, `State<T>`, `Widget::bind_state()`, `Widget::animate(dt)`
- Public API unchanged: `on_click`, constructor signature

- [ ] **Step 1: Rewrite Button.hpp**

```cpp
#pragma once
#include "../Widget.hpp"
#include "../State/AnimatedState.hpp"
#include "../State/State.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(glm::ivec4 rect, std::string label = "");

        auto update(engine::Context& context) -> void override;
        auto animate(std::chrono::milliseconds dt) -> void override;
        auto draw(engine::Context& context, backend::Backend& backend) -> void override;

        std::function<void()> on_click;
    private:
        std::string label_;

        glm::vec4 idle_f{100.0F / 255.0F, 130.0F / 255.0F, 180.0F / 255.0F, 1.0F};
        glm::vec4 hover_f{130.0F / 255.0F, 160.0F / 255.0F, 210.0F / 255.0F, 1.0F};
        glm::vec4 press_f{200.0F / 255.0F, 100.0F / 255.0F, 100.0F / 255.0F, 1.0F};

        AnimatedState<glm::vec4> fill_color_{
            idle_f, std::chrono::milliseconds(200), animation::ease_out_quad
        };
        glm::vec4 target_{idle_f};

        type::Color border_color_{60, 80, 120, 255};
        type::Color text_color_{220, 220, 230, 255};
    };
} // namespace neko::widget
```

- [ ] **Step 2: Rewrite Button.cpp**

```cpp
#include "Button.hpp"

neko::widget::Button::Button(const glm::ivec4 rect, std::string label) : label_(std::move(label)) {
    set_bounds(rect);
    fill_color_.set_observer([this](const glm::vec4&) { mark_dirty(); });
}

auto neko::widget::Button::update(engine::Context& context) -> void {
    const float s = context.dpi_scale;
    const bool hover = context.mouse.is_inside(bounds(), s);
    const bool down = hover && context.mouse.left_down;

    if (context.mouse.left_released() && hover && on_click) {
        on_click();
    }

    glm::vec4 new_target;
    if (down) {
        new_target = press_f;
    } else if (hover) {
        new_target = hover_f;
    } else {
        new_target = idle_f;
    }

    if (target_ != new_target) {
        target_ = new_target;
        if (!fill_color_.animating()) {
            context.animation_start();
        }
        fill_color_ = new_target;  // triggers animation
    }

    Widget::update(context);
}

auto neko::widget::Button::animate(const std::chrono::milliseconds dt) -> void {
    fill_color_.update(dt);
    Widget::animate(dt);
}

auto neko::widget::Button::draw(engine::Context& context, backend::Backend& backend) -> void {
    const glm::vec4 current_f = fill_color_;

    if (fill_color_.animating()) {
        context.dirty = true;
    }

    const type::Color current{
        static_cast<int>(current_f.r * 255.0F + 0.5F),
        static_cast<int>(current_f.g * 255.0F + 0.5F),
        static_cast<int>(current_f.b * 255.0F + 0.5F),
        static_cast<int>(current_f.a * 255.0F + 0.5F),
    };
    backend.draw_rect_fill(bounds(), current);
    backend.draw_rect(bounds(), border_color_, 2);

    if (!label_.empty()) {
        const auto& b = bounds();
        const int y_center = b.y + (b.w - 16) / 2;
        backend.draw_text(label_, {b.x + 8, y_center}, text_color_);
    }

    Widget::draw(context, backend);
}
```

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/Widget/Button/Button.hpp NekoUI/NekoUI/Widget/Button/Button.cpp
git commit -m "refactor: migrate Button to AnimatedState + reactive pattern"
```

---

### Task 9: Migrate Checkbox to reactive state + AnimatedState

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Checkbox/Checkbox.hpp`
- Modify: `NekoUI/NekoUI/Widget/Checkbox/Checkbox.cpp`

**Interfaces:**
- Public API unchanged: `on_toggled(bool)`, `is_checked()`, `set_checked(bool)`, `toggle()`, `focusable()`

- [ ] **Step 1: Rewrite Checkbox.hpp**

```cpp
#pragma once

#include "../Widget.hpp"
#include "../State/AnimatedState.hpp"
#include "../State/State.hpp"

#include <functional>
#include <string>

namespace neko::widget {
    class Checkbox final : public Widget {
    public:
        explicit Checkbox(glm::ivec4 bounds = {}, std::string label = "");

        auto update(engine::Context& context) -> void override;
        auto animate(std::chrono::milliseconds dt) -> void override;
        auto draw(engine::Context& context, backend::Backend& backend) -> void override;
        auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool override;

        [[nodiscard]] auto focusable() const -> bool override;
        [[nodiscard]] auto is_checked() const -> bool;

        auto set_checked(bool checked) -> void;
        auto toggle() -> void;

        std::function<void(bool)> on_toggled;
    private:
        auto toggle(engine::Context& context) -> void;

        bool m_checked = false;
        std::string m_label;

        AnimatedState<glm::vec4> m_bg_anim{{0.7F, 0.7F, 0.75F, 1.0F}, std::chrono::milliseconds(150), animation::ease_out_quad};

        glm::vec4 m_unchecked_color{0.7F, 0.7F, 0.75F, 1.0F};
        glm::vec4 m_checked_color{0.24F, 0.47F, 0.86F, 1.0F};
        type::Color m_check_color{255, 255, 255, 255};
        type::Color m_text_color{255, 255, 255, 255};
        type::Color m_normal_border{140, 140, 150, 255};
        type::Color m_focus_border{60, 120, 220, 255};
    };
} // namespace neko::widget
```

- [ ] **Step 2: Rewrite Checkbox.cpp**

```cpp
#include "Checkbox.hpp"

neko::widget::Checkbox::Checkbox(const glm::ivec4 bounds, std::string label) : m_label(std::move(label)) {
    set_bounds(bounds);
    m_bg_anim.set_observer([this](const glm::vec4&) { mark_dirty(); });
    // Set target to match initial checked state
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
}

auto neko::widget::Checkbox::update(engine::Context& context) -> void {
    Widget::update(context);
}

auto neko::widget::Checkbox::animate(const std::chrono::milliseconds dt) -> void {
    m_bg_anim.update(dt);
    Widget::animate(dt);
}

auto neko::widget::Checkbox::draw(engine::Context& context, backend::Backend& backend) -> void {
    const auto& b = bounds();
    constexpr int box = 18;
    const int bx = b.x;
    const int by = b.y + (b.w - box) / 2;

    const glm::vec4 current_f = m_bg_anim;
    const float progress = m_bg_anim.animating() ? 0.5f : 1.0f; // approximate — just use display value

    // Box background (animated color)
    const glm::ivec4 box_color{
        static_cast<int>(current_f.r * 255.0F + 0.5F),
        static_cast<int>(current_f.g * 255.0F + 0.5F),
        static_cast<int>(current_f.b * 255.0F + 0.5F),
        255,
    };

    if (m_checked || m_bg_anim.animating()) {
        backend.draw_rect_fill({bx, by, box, box}, box_color);
    }

    const auto border = has_focus() ? m_focus_border : m_normal_border;
    backend.draw_rect({bx, by, box, box}, border, 1);

    // Checkmark (white rect approximation)
    if (m_checked || m_bg_anim.animating()) {
        backend.draw_rect_fill({bx + 4, by + 4, box - 8, box - 8}, m_check_color);
    }

    // Label (vertically centered, baseline offset)
    if (!m_label.empty()) {
        constexpr int font_ascent = 13;
        const int label_y = b.y + (b.w - 16) / 2 + font_ascent;
        backend.draw_text(m_label, {bx + box + 8, label_y}, m_text_color);
    }

    // Focus border
    if (has_focus()) {
        backend.draw_rect(b, m_focus_border, 1);
    }

    Widget::draw(context, backend);
}

auto neko::widget::Checkbox::handle_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
    if (msg == WM_LBUTTONDOWN && context.mouse.is_inside(bounds(), context.dpi_scale)) {
        if (context.request_focus) {
            context.request_focus(this);
        }
        toggle(context);
        return true;
    }
    if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
        toggle(context);
        return true;
    }
    return Widget::handle_event(context, msg, wparam, lparam);
}

auto neko::widget::Checkbox::focusable() const -> bool {
    return true;
}

auto neko::widget::Checkbox::is_checked() const -> bool {
    return m_checked;
}

auto neko::widget::Checkbox::set_checked(const bool checked) -> void {
    m_checked = checked;
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
}

auto neko::widget::Checkbox::toggle() -> void {
    m_checked = !m_checked;
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
    if (on_toggled) {
        on_toggled(m_checked);
    }
}

auto neko::widget::Checkbox::toggle(engine::Context& context) -> void {
    m_checked = !m_checked;
    m_bg_anim = m_checked ? m_checked_color : m_unchecked_color;
    context.animation_start();
    context.dirty = true;
    if (on_toggled) {
        on_toggled(m_checked);
    }
}
```

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/Widget/Checkbox/Checkbox.hpp NekoUI/NekoUI/Widget/Checkbox/Checkbox.cpp
git commit -m "refactor: migrate Checkbox to AnimatedState + reactive pattern"
```

---

### Task 10: Migrate TextInput to reactive State

**Files:**
- Modify: `NekoUI/NekoUI/Widget/TextInput/TextInput.hpp`
- Modify: `NekoUI/NekoUI/Widget/TextInput/TextInput.cpp`

**Interfaces:**
- Consumes: `neko::state::State<T>` for `m_text`
- Public API unchanged: `text()`, `set_text()`, `set_placeholder()`, `on_text_changed`

- [ ] **Step 1: Modify TextInput.hpp — add State for m_text**

```cpp
#pragma once

#include "../Widget.hpp"
#include "../State/State.hpp"

#include <chrono>
#include <functional>
#include <string>
#include <utility>

namespace neko::widget {
    class TextInput final : public Widget {
    public:
        explicit TextInput(glm::ivec4 bounds = {}, std::string placeholder = "");

        auto update(engine::Context& context) -> void override;
        auto draw(engine::Context& context, backend::Backend& backend) -> void override;
        auto handle_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool override;

        [[nodiscard]] auto focusable() const -> bool override;
        auto on_focus_gained() -> void override;
        auto on_focus_lost() -> void override;

        [[nodiscard]] auto text() const -> const std::string&;
        auto set_text(std::string_view t) -> void;
        auto set_placeholder(std::string_view t) -> void;

        std::function<void(std::string_view)> on_text_changed;
    private:
        [[nodiscard]] auto has_selection() const -> bool;
        auto delete_selection() -> void;

        State<std::string> m_text;
        std::string m_placeholder;
        int m_cursor_pos = 0;
        int m_sel_start = -1;
        std::chrono::steady_clock::time_point m_cursor_tick;
        bool m_cursor_visible = true;

        type::Color m_bg_color{240, 240, 245, 255};
        type::Color m_text_color{30, 30, 30, 255};
        type::Color m_cursor_color{60, 60, 60, 255};
        type::Color m_sel_color{180, 200, 240, 255};
        type::Color m_border_color{180, 180, 190, 255};
        type::Color m_focus_border_color{60, 120, 220, 255};
        type::Color m_placeholder_color{180, 180, 180, 255};
    };
} // namespace neko::widget
```

- [ ] **Step 2: Modify TextInput.cpp — use State<std::string> for m_text**

Replace `m_text` with `m_text.get()` where reading, and `m_text = value` where writing.

Changes needed:
- Constructor: bind_state(m_text), call set_text or use `m_text = initial` (empty string)
- `draw()`: read via `m_text.get()` or implicit `m_text`
- `handle_event()`: write via `m_text = modified_value`, plus `on_text_changed`
- `set_text()`: use `m_text = t` (which triggers observer → dirty)
- `text()`: return via `m_text.get()`

Let me write the complete new .cpp:

```cpp
#include "TextInput.hpp"

neko::widget::TextInput::TextInput(const glm::ivec4 bounds, std::string placeholder) : m_placeholder(std::move(placeholder)) {
    set_bounds(bounds);
    bind_state(m_text);
}

auto neko::widget::TextInput::update(engine::Context& context) -> void {
    const auto now = std::chrono::steady_clock::now();
    if (now - m_cursor_tick > std::chrono::milliseconds(500)) {
        m_cursor_visible = !m_cursor_visible;
        m_cursor_tick = now;
        context.dirty = true;
    }
    Widget::update(context);
}

auto neko::widget::TextInput::draw(engine::Context& context, backend::Backend& backend) -> void {
    const auto& b = bounds();
    const auto& text = m_text.get();

    // Background + border
    backend.draw_rect_fill(b, m_bg_color);
    backend.draw_rect(b, has_focus() ? m_focus_border_color : m_border_color, 1);

    // Selection background
    if (has_focus() && m_sel_start >= 0 && m_sel_start != m_cursor_pos) {
        const int sel_begin = (std::min)(m_sel_start, m_cursor_pos);
        const int sel_end = (std::max)(m_sel_start, m_cursor_pos);
        backend.draw_rect_fill({b.x + 2, b.y + 2, (sel_end - sel_begin) * 8, b.w - 4}, m_sel_color);
    }

    // Text (vertically centered)
    const bool empty = text.empty();
    const auto& display_text = empty ? m_placeholder : text;
    const auto text_color = empty ? m_placeholder_color : m_text_color;
    constexpr int font_size = 16;
    constexpr int font_ascent = 13;
    const int text_y = b.y + (b.w - font_size) / 2 + font_ascent;
    backend.draw_text(display_text, {b.x + 4, text_y}, text_color);

    // Cursor
    if (has_focus() && m_cursor_visible) {
        int cx = b.x + 4 + (m_cursor_pos * 8);
        backend.draw_rect({cx, b.y + 2, 2, b.w - 4}, m_cursor_color, 1);
    }

    Widget::draw(context, backend);
}

auto neko::widget::TextInput::handle_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
    if (msg == WM_CHAR) {
        const auto ch = static_cast<char>(wparam);
        if (ch >= 32 && ch <= 126) {
            auto new_text = m_text.get();
            new_text.insert(m_cursor_pos, 1, ch);
            m_text = std::move(new_text);  // triggers observer → dirty
            ++m_cursor_pos;
            m_sel_start = -1;
            if (on_text_changed) {
                on_text_changed(m_text.get());
            }
        }
        return true;
    }

    if (msg == WM_KEYDOWN) {
        switch (wparam) {
            case VK_BACK:
                if (has_selection()) {
                    auto new_text = m_text.get();
                    const int start = (std::min)(m_sel_start, m_cursor_pos);
                    const int end = (std::max)(m_sel_start, m_cursor_pos);
                    new_text.erase(start, end - start);
                    m_text = std::move(new_text);
                    m_cursor_pos = start;
                    m_sel_start = -1;
                } else if (m_cursor_pos > 0 && !m_text.get().empty()) {
                    auto new_text = m_text.get();
                    new_text.erase(m_cursor_pos - 1, 1);
                    m_text = std::move(new_text);
                    --m_cursor_pos;
                    m_sel_start = -1;
                } else {
                    return true;
                }
                m_cursor_visible = true;
                m_cursor_tick = std::chrono::steady_clock::now();
                if (on_text_changed) {
                    on_text_changed(m_text.get());
                }
                return true;
            case VK_DELETE:
                if (has_selection()) {
                    auto new_text = m_text.get();
                    const int start = (std::min)(m_sel_start, m_cursor_pos);
                    const int end = (std::max)(m_sel_start, m_cursor_pos);
                    new_text.erase(start, end - start);
                    m_text = std::move(new_text);
                    m_cursor_pos = start;
                    m_sel_start = -1;
                } else if (std::cmp_less(m_cursor_pos, m_text.get().size())) {
                    auto new_text = m_text.get();
                    new_text.erase(m_cursor_pos, 1);
                    m_text = std::move(new_text);
                    m_sel_start = -1;
                } else {
                    return true;
                }
                m_cursor_visible = true;
                m_cursor_tick = std::chrono::steady_clock::now();
                if (on_text_changed) {
                    on_text_changed(m_text.get());
                }
                return true;
            case VK_LEFT:
                if (m_cursor_pos > 0) {
                    --m_cursor_pos;
                    m_sel_start = -1;
                    context.dirty = true;
                }
                return true;
            case VK_RIGHT:
                if (std::cmp_less(m_cursor_pos, m_text.get().size())) {
                    ++m_cursor_pos;
                    m_sel_start = -1;
                    context.dirty = true;
                }
                return true;
            case VK_HOME:
                m_cursor_pos = 0;
                m_sel_start = -1;
                context.dirty = true;
                return true;
            case VK_END:
                m_cursor_pos = static_cast<int>(m_text.get().size());
                m_sel_start = -1;
                context.dirty = true;
                return true;
            case 'A':
                if ((GetKeyState(VK_CONTROL) & 0x8000) != 0) {
                    m_sel_start = 0;
                    m_cursor_pos = static_cast<int>(m_text.get().size());
                    context.dirty = true;
                    return true;
                }
                break;
            default: ;
        }
        return true;
    }

    return Widget::handle_event(context, msg, wparam, lparam);
}

auto neko::widget::TextInput::focusable() const -> bool {
    return true;
}

auto neko::widget::TextInput::on_focus_gained() -> void {
    m_cursor_visible = true;
    m_cursor_tick = std::chrono::steady_clock::now();
}

auto neko::widget::TextInput::on_focus_lost() -> void {
    m_cursor_visible = false;
    m_sel_start = -1;
}

auto neko::widget::TextInput::text() const -> const std::string& {
    return m_text.get();
}

auto neko::widget::TextInput::set_text(const std::string_view t) -> void {
    m_text = std::string(t);  // triggers observer → dirty
    m_cursor_pos = static_cast<int>(m_text.get().size());
    m_sel_start = -1;
}

auto neko::widget::TextInput::set_placeholder(const std::string_view t) -> void {
    m_placeholder = t;
}

auto neko::widget::TextInput::has_selection() const -> bool {
    return m_sel_start >= 0 && m_sel_start != m_cursor_pos;
}

auto neko::widget::TextInput::delete_selection() -> void {
    const int start = (std::min)(m_sel_start, m_cursor_pos);
    const int end = (std::max)(m_sel_start, m_cursor_pos);
    auto new_text = m_text.get();
    new_text.erase(start, end - start);
    m_text = std::move(new_text);
    m_cursor_pos = start;
}
```

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/Widget/TextInput/TextInput.hpp NekoUI/NekoUI/Widget/TextInput/TextInput.cpp
git commit -m "refactor: migrate TextInput to State<std::string>"
```

