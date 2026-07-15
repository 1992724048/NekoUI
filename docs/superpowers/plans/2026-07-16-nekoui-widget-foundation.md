# NekoUI Widget 基础体系 + 渲染 + 动画接入 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 让 Button 能加入控件树、渲染文本与颜色、响应点击、并能动画变色；提供 Container 布局与 main.cpp 示例，形成可运行的垂直切片。

**Architecture:** Widget 持有 `children_` 容器（每 Widget 一把 `shared_mutex` 保护结构与属性，单写者模型：所有变更在消息线程，渲染线程只读）。`add<T>()` 工厂递归构建树；基类 `draw()` 先 `draw_self()` 再递归子节点；`layout()` 递归设置 bounds。`Animation<T>` 增加 `on_update` 回调，Widget 在 `draw_self` 中 tick 动画并应用属性，通过 Context 回调驱动按需重绘。

**Tech Stack:** C++23 (C++latest), DirectX 11, Win32 API, Intel C++ 2026 编译器。无第三方测试框架（测试为后续独立 plan）。

## Global Constraints

- 语言标准：C++latest（C++23+），积极使用 `std::jthread`/`std::span`/`std::optional`/`requires` 等
- 命名：类/struct PascalCase；函数/方法 snake_case 后缀 `_returntype`（如 `set_text() -> void`）；namespace snake_case；字段 snake_case 带 `_`
- 注释：命名优先于注释；禁止废话注释（描述"做什么"）、日志式注释、装饰性分隔符
- 组合优于继承；面向接口；单一职责
- 零警告编译（提交前无 warning）
- 圈复杂度 V(G) ≤ 10，超标必须拆分
- 项目用 Intel C++ 2026 + C++latest；AI 不编译，由用户在 VS (Debug|x64) 验证
- 线程模型：消息线程改 Widget，渲染线程读 Widget；每 Widget 一把 `mutable std::shared_mutex mutex_` 保护所有可变状态
- 提交信息用 Conventional Commits 格式（`<type>(<scope>): <描述>`），正文中文

---

## File Structure

| 文件 | 职责 | 操作 |
|------|------|------|
| `NekoUI/NekoUI/Widget/Widget.hpp` | 控件基类：children 容器、add<T>、递归 layout/draw、mutex、id setter | Modify |
| `NekoUI/NekoUI/Widget/Widget.cpp` | layout/draw 递归实现、id setter | Modify |
| `NekoUI/NekoUI/Widget/Button/Button.hpp` | Button：text/color 属性、双构造、draw_self | Modify |
| `NekoUI/NekoUI/Widget/Button/Button.cpp` | draw_self（矩形+文本）、点击动画触发 | Modify |
| `NekoUI/NekoUI/Widget/Container/Container.hpp` | 容器：垂直/水平布局方向 | Create |
| `NekoUI/NekoUI/Widget/Container/Container.cpp` | layout 按方向排列子节点 | Create |
| `NekoUI/NekoUI/Engine/Component/Animation.hpp` | 增加 on_update 回调，tick 中调用 | Modify |
| `NekoUI/NekoUI/main.cpp` | 示例：构建树、Button 点击动画 | Modify |
| `NekoUI/NekoUI/NekoUI.vcxproj` / `.filters` | 新增 Container 文件登记 | Modify |

---

## Task 1: Widget 控件树（children + add<T> + 递归 layout/draw + mutex）

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Widget.hpp`
- Modify: `NekoUI/NekoUI/Widget/Widget.cpp`

**Interfaces:**
- Consumes: `engine::Context`（已存在）、`backend::Backend`（已存在）、`mouse::Mouse`（已存在）
- Produces: `add<T>()` 工厂（返回 `std::shared_ptr<T>`）、`draw_self()` 虚钩子、`set_id()`、`children_count()`

- [ ] **Step 1: 修改 Widget.hpp — 增加容器、mutex、add<T>、draw_self、set_id**

在 `Widget.hpp` 顶部增加 include：
```cpp
#include <vector>
#include <shared_mutex>
#include <utility>
```

在 `protected:` 区增加（在 `engine::Context* context{};` 附近）：
```cpp
    mutable std::shared_mutex mutex_{};
    std::vector<std::shared_ptr<Widget>> children_{};
```

在 `public:` 区，将 `draw` / `layout` 改为非 virtual 的递归入口，并新增 `draw_self` 虚钩子：
```cpp
    virtual auto draw_self(engine::Context& context, backend::Backend& backend) -> void {}
    auto draw(engine::Context& context, backend::Backend& backend) -> void;
    auto layout(Constraints constraints) -> void;

    template<typename T, typename... Args>
    requires std::is_base_of_v<Widget, T>
    auto add(Args&&... args) -> std::shared_ptr<T> {
        auto child = std::make_shared<T>(this, std::forward<Args>(args)...);
        {
            std::unique_lock lock(mutex_);
            children_.push_back(child);
        }
        if (context) {
            context->reg_widget(child);
        }
        return child;
    }

    auto set_id(std::string_view id) -> void;
    [[nodiscard]] auto children_count() const -> size_t;
```

将原有 `virtual auto draw(...)` 和 `virtual auto layout(...)` 从类定义中移除（改为上面非 virtual 声明）。保留 `raw_event` / `hit_test` 为 virtual（子类按需重写）。

- [ ] **Step 2: 修改 Widget.cpp — 实现递归 layout/draw、set_id、children_count**

```cpp
namespace neko::widget {
    Widget::Widget(engine::Context& context)
        : context{&context}, root{context.root} {}

    Widget::Widget(Widget* parent)
        : context{parent ? parent->context : nullptr},
          parent{parent},
          root{parent ? parent->root.load() : std::weak_ptr<Widget>{}} {}

    Widget::~Widget() = default;

    auto Widget::set_id(const std::string_view id) -> void {
        std::unique_lock lock(mutex_);
        id_ = id;
    }

    auto Widget::children_count() const -> size_t {
        std::shared_lock lock(mutex_);
        return children_.size();
    }

    auto Widget::layout(Constraints constraints) -> void {
        {
            std::unique_lock lock(mutex_);
            bounds = Vec4I{constraints.x, constraints.y, constraints.width, constraints.height};
        }
        std::shared_lock lock(mutex_);
        for (const auto& child : children_) {
            child->layout(Constraints{
                .x = constraints.x,
                .y = constraints.y,
                .width = constraints.width,
                .height = constraints.height});
        }
    }

    auto Widget::draw(engine::Context& context, backend::Backend& backend) -> void {
        draw_self(context, backend);
        std::shared_lock lock(mutex_);
        for (const auto& child : children_) {
            child->draw(context, backend);
        }
    }

    auto Widget::raw_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
        // 默认：先派发给子节点（后加入的在上层，逆序命中测试）
        std::shared_lock lock(mutex_);
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            if ((*it)->raw_event(context, msg, wparam, lparam)) {
                return true;
            }
        }
        return false;
    }

    auto Widget::hit_test(const mouse::Mouse& mouse) const -> bool {
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

    auto Widget::id() const -> const std::string& {
        std::shared_lock lock(mutex_);
        return id_;
    }
}
```

注意：`raw_event` / `hit_test` 原在 .hpp 内联，现移到 .cpp 并实现递归派发。`bounds` 为 `Vec4I`（int），`mouse.is_inside(Vec4I)` 已存在。

- [ ] **Step 3: 在 VS 中编译验证（Debug|x64）**

用户执行：在 VS 中 Build `NekoUI` (Debug|x64)。
预期：编译通过，无 warning。（AI 不编译，由用户验证）

- [ ] **Step 4: Commit**

```bash
git add NekoUI/NekoUI/Widget/Widget.hpp NekoUI/NekoUI/Widget/Widget.cpp
git commit -m "feat(widget): 增加 children 容器与 add<T> 工厂，递归 layout/draw

Widget 持有 children_ 容器，每实例一把 shared_mutex 保护结构与属性。
add<T>() 构造子控件并注册到 WidgetTree。基类 draw/layout 递归遍历子节点，
draw_self 作为子类自绘钩子。修复 id_ 无 setter 问题。"
```

---

## Task 2: Button 文本/颜色属性 + 自绘

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Button/Button.hpp`
- Modify: `NekoUI/NekoUI/Widget/Button/Button.cpp`

**Interfaces:**
- Consumes: `Widget` 基类（Task 1 的 `add`/递归 draw）、`backend::Backend::draw_rect_fill` / `draw_text`、`Type.hpp` 的 `Color` / `Vec4I`
- Produces: `set_text()` / `text()` / `set_color()` / `color()`、`on_click` 回调、`animate_color()`

- [ ] **Step 1: 修改 Button.hpp — 属性、双构造、on_click、draw_self 声明**

```cpp
#pragma once
#include "../../Widget.hpp"
#include <string>
#include <functional>

namespace neko::widget {
    class Button final : public Widget {
    public:
        explicit Button(engine::Context& context, Vec4I bounds = {}, std::string_view label = "");
        explicit Button(Widget* parent, Vec4I bounds = {}, std::string_view label = "");

        auto set_text(std::string_view text) -> void;
        [[nodiscard]] auto text() const -> std::string;
        auto set_color(Color color) -> void;
        [[nodiscard]] auto color() const -> Color;
        auto set_on_click(std::function<void()> callback) -> void;
        auto animate_color(Color target, int duration_ms = 200) -> void;

    protected:
        auto draw_self(engine::Context& context, backend::Backend& backend) -> void override;
        auto raw_event(engine::Context& context, UINT msg, WPARAM wparam, LPARAM lparam) -> bool override;

    private:
        std::string text_{};
        Color color_{0xFF4A90D9};
        std::function<void()> on_click_{};
        animation::Animation<int, animation::ease::quad::in_out> color_anim_{0, 200};
    };
}
```

注意：`color_anim_` 类型 `Animation<int, ...>` 来自 `Engine/Component/Animation.hpp`，需在 Button.hpp 包含它（或前置声明 + 在 .cpp 包含）。为简化，Button.hpp 包含 `../../Engine/Component/Animation.hpp`。但 Widget.hpp 不应反向依赖 Engine/Component（保持 Widget 不依赖 Engine 内部）。可接受：Button 是具体控件，可依赖 Animation。

- [ ] **Step 2: 修改 Button.cpp — 构造、属性、draw_self、点击**

```cpp
#include "Button.hpp"
#include "../../Engine/Component/Animation.hpp"

namespace neko::widget {
    Button::Button(engine::Context& context, const Vec4I bounds, const std::string_view label)
        : Widget{context}, text_{label.data(), label.size()}, color_anim_{0, 200} {
        this->bounds = bounds;
    }

    Button::Button(Widget* parent, const Vec4I bounds, const std::string_view label)
        : Widget{parent}, text_{label.data(), label.size()}, color_anim_{0, 200} {
        this->bounds = bounds;
    }

    auto Button::set_text(const std::string_view text) -> void {
        {
            std::unique_lock lock(mutex_);
            text_ = text;
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
        // 将 Color 映射为 int（0xRRGGBBAA）做插值
        const int from = static_cast<int>(color_.value);
        color_anim_ = static_cast<int>(target.value);
        color_anim_.to_value(static_cast<int>(target.value),
                             duration_ms > 0 ? std::optional<std::chrono::milliseconds>{duration_ms}
                                            : std::nullopt);
        color_anim_.on_update = [this](const int v) {
            {
                std::unique_lock lock(mutex_);
                color_ = Color{static_cast<uint32_t>(v)};
            }
            if (context) {
                context->mark_dirty();
            }
        };
    }

    auto Button::draw_self(engine::Context& ctx, backend::Backend& backend) -> void {
        std::shared_lock lock(mutex_);
        backend.draw_rect_fill(bounds, color_);
        if (!text_.empty()) {
            const Vec2I pos{bounds.x + 8, bounds.y + (bounds.height - 16) / 2};
            backend.draw_text(text_, pos, Color{0xFF000000}, 16.0F);
        }
        // 动画推进（渲染线程 tick）
        if (color_anim_.is_active()) {
            color_anim_();  // tick，触发 on_update 更新 color_
        }
    }

    auto Button::raw_event(engine::Context& ctx, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
        if (msg == WM_LBUTTONUP) {
            std::shared_lock lock(mutex_);
            if (on_click_) {
                on_click_();
            }
            return true;
        }
        return Widget::raw_event(ctx, msg, wparam, lparam);
    }
}
```

注意：`color_anim_()` 即 `operator()` = `tick()`，返回当前插值并触发 `on_update`。`on_update` 在 Task 4 加入 Animation。若 Task 4 未先做，本任务编译会失败（无 `on_update` 成员）——**Task 2 依赖 Task 4 先合入**，或合并实现。建议执行顺序：先 Task 4（Animation on_update），再 Task 2。

- [ ] **Step 3: 在 VS 中编译验证（Debug|x64）**

用户执行：Build `NekoUI` (Debug|x64)。
预期：编译通过，无 warning。

- [ ] **Step 4: Commit**

```bash
git add NekoUI/NekoUI/Widget/Button/Button.hpp NekoUI/NekoUI/Widget/Button/Button.cpp
git commit -m "feat(button): 增加 text/color 属性与自绘，点击触发 on_click

Button 重写 draw_self 绘制矩形+文本，raw_event 在 WM_LBUTTONUP 触发 on_click。
提供 set_text/set_color/set_on_click/animate_color 接口。"
```

---

## Task 3: Container 布局控件（垂直/水平）

**Files:**
- Create: `NekoUI/NekoUI/Widget/Container/Container.hpp`
- Create: `NekoUI/NekoUI/Widget/Container/Container.cpp`
- Modify: `NekoUI/NekoUI/NekoUI.vcxproj` + `.filters`（登记新文件）

**Interfaces:**
- Consumes: `Widget` 基类（Task 1 的 children/add/递归）
- Produces: `Container` 控件、`set_direction()` / `direction()`

- [ ] **Step 1: 创建 Container.hpp**

```cpp
#pragma once
#include "../../Widget.hpp"

namespace neko::widget {
    enum class LayoutDirection { Vertical, Horizontal };

    class Container : public Widget {
    public:
        using Widget::Widget;

        auto set_direction(LayoutDirection dir) -> void;
        [[nodiscard]] auto direction() const -> LayoutDirection;

    protected:
        auto layout(Constraints constraints) -> void override;

    private:
        LayoutDirection direction_{LayoutDirection::Vertical};
    };
}
```

- [ ] **Step 2: 创建 Container.cpp**

```cpp
#include "Container.hpp"

namespace neko::widget {
    auto Container::set_direction(const LayoutDirection dir) -> void {
        {
            std::unique_lock lock(mutex_);
            direction_ = dir;
        }
        if (context) {
            context->mark_dirty();
        }
    }

    auto Container::direction() const -> LayoutDirection {
        std::shared_lock lock(mutex_);
        return direction_;
    }

    auto Container::layout(Constraints constraints) -> void {
        {
            std::unique_lock lock(mutex_);
            bounds = Vec4I{constraints.x, constraints.y, constraints.width, constraints.height};
        }
        std::shared_lock lock(mutex_);
        const size_t n = children_.size();
        if (n == 0) {
            return;
        }
        if (direction_ == LayoutDirection::Vertical) {
            const int h = constraints.height / static_cast<int>(n);
            int y = constraints.y;
            for (const auto& child : children_) {
                child->layout(Constraints{.x = constraints.x, .y = y, .width = constraints.width, .height = h});
                y += h;
            }
        } else {
            const int w = constraints.width / static_cast<int>(n);
            int x = constraints.x;
            for (const auto& child : children_) {
                child->layout(Constraints{.x = x, .y = constraints.y, .width = w, .height = constraints.height});
                x += w;
            }
        }
    }
}
```

- [ ] **Step 3: 登记到 vcxproj / filters**

在 `NekoUI/NekoUI/NekoUI.vcxproj` 的 `<ClCompile Include="NekoUI\Widget\Button\Button.cpp" />` 附近添加：
```xml
<ClCompile Include="NekoUI\Widget\Container\Container.cpp" />
```
在 `NekoUI/NekoUI/NekoUI.vcxproj.filters` 对应 `<ClCompile>` Filter 组添加：
```xml
<ClCompile Include="NekoUI\Widget\Container\Container.cpp">
  <Filter>Source Files\Widget\Container</Filter>
</ClCompile>
```
并在 `<ClInclude>` 组添加：
```xml
<ClInclude Include="NekoUI\Widget\Container\Container.hpp">
  <Filter>Header Files\Widget\Container</Filter>
</ClInclude>
```
（Filter 路径按现有约定调整；若项目无 Filter 分组则仅加 `<ClCompile>`/`<ClInclude>` 即可）

- [ ] **Step 4: 在 VS 中编译验证（Debug|x64）**

用户执行：Build `NekoUI` (Debug|x64)。
预期：编译通过，Container 文件已登记，无 warning。

- [ ] **Step 5: Commit**

```bash
git add NekoUI/NekoUI/Widget/Container/Container.hpp NekoUI/NekoUI/Widget/Container/Container.cpp NekoUI/NekoUI/NekoUI.vcxproj NekoUI/NekoUI/NekoUI.vcxproj.filters
git commit -m "feat(widget): 增加 Container 布局控件（垂直/水平）

Container 重写 layout，按 direction_ 等分可用空间排列子节点。
登记新文件到 vcxproj/filters。"
```

---

## Task 4: Animation on_update 回调 + Context 自动绑定

**Files:**
- Modify: `NekoUI/NekoUI/Engine/Component/Animation.hpp`

**Interfaces:**
- Consumes: `AnimationBase`（已存在 `bind`/`inc_`/`dec_`）、`Context` 回调（通过 Widget 设置 on_update 间接使用）
- Produces: `on_update` 回调成员、`tick()` 中调用 on_update

- [ ] **Step 1: 修改 Animation.hpp — 增加 on_update 成员**

在 `Animation` 模板类的 private 成员区增加：
```cpp
    std::function<void(T)> on_update_{nullptr};
```

在 public 区增加 setter：
```cpp
    auto on_update(std::function<void(T)> callback) -> void { on_update_ = std::move(callback); }
```

- [ ] **Step 2: 修改 tick() — 计算后调用 on_update**

找到 `Animation<T>::tick()` 实现（返回 `now_value_` 处），在计算完 `now_value_` 之后、返回之前插入：
```cpp
    if (on_update_) {
        on_update_(now_value_);
    }
```
完整 tick 示意（保持原有插值逻辑，仅追加 on_update 调用）：
```cpp
    auto tick() -> T {
        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<TimeType>(now - start_);
        if (elapsed >= duration_time_) {
            now_value_ = new_value_;
            change_ = false;
            if (dec_) dec_();
        } else {
            const float t = EasingFnType(static_cast<float>(elapsed.count()) /
                                          static_cast<float>(duration_time_.count()));
            now_value_ = static_cast<T>(start_value_ + (new_value_ - start_value_) * t);
        }
        if (on_update_) {
            on_update_(now_value_);
        }
        return now_value_;
    }
```

注意：`operator()()` 与 `operator T()` 均委托 `tick()`，故它们也会触发 on_update。

- [ ] **Step 3: 在 VS 中编译验证（Debug|x64）**

用户执行：Build `NekoUI` (Debug|x64)。
预期：编译通过，无 warning。

- [ ] **Step 4: Commit**

```bash
git add NekoUI/NekoUI/Engine/Component/Animation.hpp
git commit -m "feat(animation): 增加 on_update 回调，tick 中触发

Animation<T> 新增 on_update 回调，每帧 tick 计算新值后调用，
使动画值变化能驱动 Widget 属性更新与重绘（配合 Context::mark_dirty）。"
```

---

## Task 5: main.cpp 示例（构建树 + 点击动画）

**Files:**
- Modify: `NekoUI/NekoUI/main.cpp`

**Interfaces:**
- Consumes: `Engine::set_root_widget<T>()`、`Container`、`Button`、`animate_color()`

- [ ] **Step 1: 修改 main.cpp — 构建控件树**

将现有 `set_root_widget<Button>(...)` 替换为：
```cpp
auto root = engine->set_root_widget<neko::widget::Container>(Vec4I{0, 0, 800, 600});
root->set_direction(neko::widget::LayoutDirection::Vertical);

auto btn1 = root->add<neko::widget::Button>(Vec4I{100, 100, 200, 50}, "点我变色");
btn1->set_on_click([btn1] {
    static bool toggle = false;
    btn1->animate_color(toggle ? Color{0xFF4A90D9} : Color{0xFFE74C3C}, 300);
    toggle = !toggle;
});

auto btn2 = root->add<neko::widget::Button>(Vec4I{100, 200, 200, 50}, "第二个");
btn2->set_on_click([btn2] {
    btn2->set_text("已点击");
});
```

注意：`set_root_widget<Container>` 调用 `Container(*context, Vec4I{...})`——Container 使用 `using Widget::Widget;` 继承 `Widget(engine::Context&)`，但缺少 `Vec4I` 参数的构造函数。需为 Container 增加 `Container(engine::Context&, Vec4I bounds = {})` 构造，或改用 `root->add<Container>()` 嵌套。简化：让 `set_root_widget<Container>()` 不带 bounds 参数（根由渲染尺寸决定），示例改为：
```cpp
auto root = engine->set_root_widget<neko::widget::Container>();
root->set_direction(neko::widget::LayoutDirection::Vertical);
auto btn1 = root->add<neko::widget::Button>(Vec4I{100, 100, 200, 50}, "点我变色");
...
```
即 `set_root_widget<Container>()` 调用 `Container(*context)`（继承的 `Widget(engine::Context&)`），bounds 由 layout 从渲染尺寸填充。

- [ ] **Step 2: 在 VS 中编译 + 运行验证**

用户执行：Build + Run `NekoUI` (Debug|x64)。
预期：
- 窗口显示两个按钮（垂直排列）
- 点击"点我变色" → 按钮颜色在 300ms 内平滑过渡（蓝↔红）
- 点击"第二个" → 文本变为"已点击"
- 空闲时 CPU 占用低（按需渲染，无空转）

- [ ] **Step 3: Commit**

```bash
git add NekoUI/NekoUI/main.cpp
git commit -m "demo: main.cpp 构建 Container+Button 树，点击触发颜色动画与文本更新

验证 Widget 树、递归 layout/draw、Animation on_update 驱动重绘的端到端链路。"
```

---

## Self-Review

**1. Spec coverage（对照设计文档 section 7 + 9）：**
- 7.1 Widget 体系（Container/Button/Label...）：本 plan 覆盖 Container + Button；Label/Image/Input 为后续 plan ✅（部分）
- 7.2 Layout 系统：Task 3 Container 布局 ✅
- 7.3 Style/CSS 接入：❌ 未覆盖（后续独立 plan）
- 7.4 Animation 接入：Task 4 + Task 2 animate_color ✅
- 7.5 渲染命令抽象：Backend 已是 immediate mode（explorer 确认），本 plan 直接使用 draw_rect_fill/draw_text，未改 Backend ✅（沿用）
- 7.6 输入事件完善：Task 2 Button 点击 + Widget 递归 raw_event/hit_test；hit_test 几何（圆角/圆/多边形）未覆盖（后续 plan）✅（部分）
- 7.7 示例/测试：Task 5 示例；gtest 未引入（后续 plan）✅（部分）
- 9.1 跨线程属性安全：Task 1 每 Widget shared_mutex 单写者模型 ✅
- 9.2 渲染命令批处理：❌ 未覆盖（非阻塞，后续 plan）
- 9.3 Layout 算法：Task 3 单阶段等分 ✅（两阶段 measure/arrange 为后续增强）
- 9.4 CSS 映射：❌ 未覆盖（后续 plan）
- 9.5 Animation→Widget 驱动：Task 4 on_update + Task 2 animate_color ✅

**2. Placeholder scan：** 无 TBD/TODO/占位。所有步骤含完整代码。Task 3 的 vcxproj Filter 路径标注"按现有约定调整"，属合理弹性（非占位）。

**3. Type consistency：**
- `add<T>()` 在 Task 1 定义，Task 2/3/5 使用 ✅
- `draw_self` 在 Task 1 声明为 virtual 钩子，Task 2 Button 重写、Task 3 Container 不重写（用默认空）✅
- `Animation::on_update` 在 Task 4 定义，Task 2 Button 使用 ✅（执行顺序需 Task 4 先于 Task 2）
- `Color{0xRRGGBBAA}` 聚合初始化（Type.hpp 无构造函数，explorer 确认）✅
- `Vec4I` 用于 bounds/构造参数 ✅
- `context->mark_dirty()` / `reg_widget()` 回调（Context.hpp 已存在）✅

**执行顺序建议：** Task 1 → Task 4 → Task 2 → Task 3 → Task 5（Task 2 依赖 Task 4 的 on_update）。

**后续独立 plan（本 plan 未覆盖）：**
- `nekoui-css-style.md`：CSS.hpp 接入 Widget 属性系统
- `nekoui-input-events.md`：hit_test 几何增强（圆角/圆/多边形）、focus 管理、键盘导航（Tab 顺序）
- `nekoui-testing.md`：引入 gtest，为 Widget 树/Animation 数学/Layout 边界写单元测试
