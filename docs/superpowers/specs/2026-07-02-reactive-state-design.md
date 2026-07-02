# Reactive State System for NekoUI

> Transition from immediate-mode widget state to retained-mode reactive state.

**Date:** 2026-07-02
**Status:** Draft
**Author:** Orchestrator + User

---

## 1. Motivation

NekoUI's widgets currently manage state manually: color members, animation objects, and dirty flags are all hand-wired. This works but doesn't scale. A reactive state system brings:

- **Automatic dirty tracking** — state change → widget re-render
- **Declarative animations** — assign a value, animation happens automatically
- **Derived state** — `Computed<T>` that invalidates on dependency changes
- **Consistent pattern** — all widgets follow the same state management pattern

---

## 2. File Structure

```
NekoUI/NekoUI/Widget/
├── State/
│   ├── State.hpp          # State<T> core
│   ├── Computed.hpp       # Computed<T> derived state
│   └── AnimatedState.hpp  # AnimatedState<T, EaseT>
├── Widget.hpp/.cpp        # + bind_state<T>(), + animate(dt)
├── Button/Button.hpp      # migrate → AnimatedState
├── Checkbox/Checkbox.hpp  # migrate → State + AnimatedState
├── TextInput/             # migrate → State
└── Component/Animation.hpp# unchanged (easing templates reused)
```

---

## 3. `State<T>` — Core Reactive Primitive

```cpp
// Widget/State/State.hpp
namespace neko::state {

template<typename T>
class State {
    T value_;
    std::function<void(const T&)> observer_;
public:
    State() = default;
    State(T val) : value_(std::move(val)) {}

    // Write — triggers observer only on actual change
    auto operator=(const T& new_val) -> State& {
        if (value_ != new_val) [[likely]] {
            value_ = new_val;
            if (observer_) observer_(value_);  // → mark dirty
        }
        return *this;
    }

    // Read — const ref, no observer trigger
    auto get() const -> const T& { return value_; }
    operator const T&() const { return value_; }

    void set_observer(std::function<void(const T&)> obs) {
        observer_ = std::move(obs);
    }
};

} // namespace neko::state
```

**Key design decisions:**
- Value comparison before firing observer prevents unnecessary re-renders
- Read is const and doesn't touch observer — thread-safe by design
- No global tracker — simple and predictable

---

## 4. Widget Integration

```cpp
// Widget.hpp additions
class Widget {
protected:
    template<typename T>
    void bind_state(State<T>& state) {
        state.set_observer([this](const T&) { mark_dirty(); });
    }
};
```

Widget subclasses call `bind_state()` for each `State` member in their constructor. This wires the observer to mark the widget dirty on change.

---

## 5. `Computed<T>` — Derived State

```cpp
// Widget/State/Computed.hpp
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

**Behavior:**
- Lazy evaluation — recomputed only when `get()` is called and deps have changed
- Dependencies declared explicitly via `depends_on()`
- Invalidated states cascade: dependent Computed → dependent Computed

---

## 6. Standalone Easing Functions

The existing `Animation.hpp` has 30 easing classes that are stateful (`Animation<T>` subclass with `update()`, `progress()`, `to_value()`, `is_done()`). These are unsuitable for lightweight per-frame interpolation in AnimatedState. Instead, we add **standalone easing functions** to `Animation.hpp`:

```cpp
// Added to Widget/Component/Animation.hpp
namespace neko::animation {

// Linear
inline auto linear(float t) -> float { return t; }

// Quadratic
inline auto ease_in_quad(float t) -> float { return t * t; }
inline auto ease_out_quad(float t) -> float { return 1.0f - (1.0f - t) * (1.0f - t); }
inline auto ease_in_out_quad(float t) -> float {
    return t < 0.5f ? 2.0f * t * t : 1.0f - (-2.0f * t + 2.0f) * (-2.0f * t + 2.0f) * 0.5f;
}

// Cubic
inline auto ease_in_cubic(float t) -> float { return t * t * t; }
inline auto ease_out_cubic(float t) -> float { return 1.0f - (1.0f - t) * (1.0f - t) * (1.0f - t); }

// ... all 30 easings as `float -> float` functions

// Bounce (already has out_bounce_impl)
inline auto ease_out_bounce(float t) -> float { return out_bounce_impl(t); }

} // namespace neko::animation
```

These functions are pure math — no state, no clock, reusable from anywhere. The existing `Animation` subclasses remain unchanged (they call `this->progress()` then apply the same formula internally).

This extraction is a **prerequisite** for AnimatedState.

---

## 7. `AnimatedState<T>` — State with Animation

```cpp
// Widget/State/AnimatedState.hpp
namespace neko::state {

template<typename T, typename TimeType = std::chrono::milliseconds>
class AnimatedState {
    using EasingFn = float(*)(float);

    State<T> current_;        // actual displayed value
    T target_, start_;        // animation endpoints
    TimeType duration_;
    TimeType elapsed_{0};
    EasingFn easing_ = animation::ease_out_quad;  // default easing
    bool animating_ = false;
    Context* ctx_ = nullptr;  // for animation_start/end

public:
    AnimatedState(T init, TimeType dur, EasingFn ease = animation::ease_out_quad)
        : current_(init), target_(init), start_(init), duration_(dur), easing_(ease) {}

    // Change easing after construction
    auto set_easing(EasingFn fn) -> void { easing_ = fn; }

    // Assignment = trigger animation from current to target
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

    // Drive interpolation (called from render thread)
    void update(TimeType dt) {
        if (!animating_) return;          // no-op when idle
        elapsed_ += dt;
        float t = std::min(1.0f, static_cast<float>(elapsed_.count())
                                 / static_cast<float>(duration_.count()));
        float eased = easing_(t);
        current_ = lerp(start_, target_, eased);
        if (t >= 1.0f) {
            current_ = target_;          // snap
            animating_ = false;
            if (ctx_) ctx_->animation_end();
        }
    }

    // State-like interface
    void set_observer(auto obs) { current_.set_observer(std::move(obs)); }
    auto get() const -> const T& { return current_.get(); }
    operator const T&() const { return get(); }
    auto animating() const -> bool { return animating_; }
    void set_context(Context& ctx) { ctx_ = &ctx; }
};

} // namespace neko::state
```

**Behavior:**
- `foo = new_value` automatically starts animation from current displayed value
- Multiple rapid assignments: animation re-targets from current interpolated value
- Easing selected by function pointer (`animation::ease_out_quad`, `animation::linear`, etc.)
- `lerp()` must be specialized per type (glm types via component-wise via GLM `mix()`, arithmetic via linear)

### lerp() Specializations

```cpp
// Built-in lerp for common types
template<typename T>
auto lerp(const T& a, const T& b, float t) -> T {
    return a + (b - a) * t;  // arithmetic types
}

// glm::vec4, glm::ivec4 — use glm::mix() or component-wise
auto lerp(const glm::vec4& a, const glm::vec4& b, float t) -> glm::vec4 {
    return glm::mix(a, b, t);
}
auto lerp(const glm::ivec4& a, const glm::ivec4& b, float t) -> glm::ivec4 {
    return glm::ivec4(glm::mix(glm::vec4(a), glm::vec4(b), t));
}
```

---

## 8. Render Thread Animation Driver

### New virtual method

```cpp
class Widget {
    // Called on render thread, per frame, before layout+draw
    virtual void animate(TimeType dt) {
        for (auto& child : children_) {
            if (child->visible()) child->animate(dt);
        }
    }
};
```

### Engine render loop change

```
render_frame(dt):
  resize if pending
  begin()
  for root in roots_:
    root->animate(dt)           ← NEW
    root->layout(constraints)
    root->draw(context, backend)
  end() → Present
```

### Widget override example

```cpp
class Button : public Widget {
    AnimatedState<type::Color> fill_color_{
        {220, 220, 220, 255}, 200ms, animation::ease_out_quad
    };
    
    void animate(TimeType dt) override {
        fill_color_.update(dt);
        Widget::animate(dt);  // children
    }
};
```

### Animation lifecycle coordination

- `AnimatedState` calls `ctx_->animation_start()` when animation begins (on assign)
- `AnimatedState` calls `ctx_->animation_end()` when animation completes
- Existing `Context::animation_refcount_` keeps render thread alive during animations
- Engine checks refcount: `wait_on_condvar` only when `!dirty && refcount == 0 && !resize_pending`

---

## 9. Thread Safety Model

| Operation | Thread | Notes |
|-----------|--------|-------|
| `State::operator=(new_val)` | msg thread | Triggers observer → mark dirty |
| `State::get()` / implicit read | either | Const ref, no side effects |
| `AnimatedState::update(dt)` | render thread | Interpolates current_ toward target_ |
| `Computed::get()` | either | Thread-safe if deps are stable during read |

**Guarantees:**
- msg thread and render thread never access the same State concurrently (rendezvous via condvar)
- AnimatedState's `current_` is written on render thread, read on render thread — no cross-thread issue
- `target_` is written on msg thread, read on render thread — but only during animation where happens-before is guaranteed by condvar sync

---

## 10. Migration Plan

### Button (priority: high)
| Current | Replace with |
|---------|-------------|
| `type::Color fill_color_{220,220,220,255}` | `AnimatedState<type::Color> fill_color_{{220,220,220,255}, 200ms, animation::ease_out_quad}` |
| `type::Color label_color_{30,30,30,255}` | `State<type::Color> label_color_{{30,30,30,255}}` |
| `EaseOutQuadAnimation<glm::vec4> color_anim_{}` | removed (integrated into AnimatedState) |
| `glm::vec4 current_color_f_` | removed |
| Manual anim logic in `update()` | `fill_color_.update(dt)` in `animate()` |
| Manual `context.dirty = true` | automatic via `bind_state()` |

### Checkbox (priority: high)
| Current | Replace with |
|---------|-------------|
| `type::Color fill_color_{240,240,240,255}` | `AnimatedState<type::Color> fill_color_{{240,240,240,255}, 200ms, animation::ease_out_quad}` |
| `type::Color check_color_{0,0,0,0}` | `AnimatedState<type::Color> check_color_{{0,0,0,0}, 150ms, animation::ease_in_quad}` |
| Manual color logic in `update()` | `bind_state()` + `animate()` |

### TextInput (priority: medium)
| Current | Replace with |
|---------|-------------|
| `std::wstring text_` | `State<std::wstring> text_` |
| `std::wstring placeholder_` | `State<std::wstring> placeholder_` (const after init → simple member) |
| `size_t cursor_pos_` | State or keep simple (cursor is rapidly updated, may be overhead) |

---

## 11. Open Questions / Future Work

- **All 30 easing functions** — Need to extract all easing formulas into `float->float` functions in Animation.hpp
- **`AnimatedState::set_context()`** — How does AnimatedState get the Context pointer? Option: pass via `bind_state()` or widget constructor injects it.
- **Batch updates** — Multiple state changes in one msg dispatch should coalesce dirty flag (already handled by bool — first set dirties, subsequent are no-ops until consumed).
- **TextInput cursor blink** — Currently timer-based; could be converted to a looping AnimatedState with 500ms half-period.
