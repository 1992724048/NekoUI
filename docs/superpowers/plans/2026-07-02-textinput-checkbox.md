# TextInput + Checkbox 实现计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans.

**Goal:** 添加焦点管理、TextInput（单行文本输入框）和 Checkbox（复选框）组件。

**Architecture:** Engine 统一管理焦点（`m_focused_widget`），键盘消息路由到焦点控件。Widget 基类新增 `focusable()`/`on_focus_gained/lost()` 虚方法。TextInput 和 Checkbox 继承 Widget，实现自身绘制和事件处理。

**Tech Stack:** C++26, DirectX 11, GLM, MSBuild

## Global Constraints

- 尾置返回类型: `auto foo() -> void`
- 命名空间: `neko::widget`, `neko::engine`
- Bounds: `glm::ivec4(x, y, width, height)`
- `type::Color` = `glm::ivec4` (RGBA, 0-255)
- 组件文件放在 `Widget/Component/` 目录

---

## 文件结构

| 文件 | 操作 | 职责 |
|------|------|------|
| `NekoUI/NekoUI/Widget/Widget.hpp` | 修改 | 添加 focus 系统：focusable/on_focus_gained/on_focus_lost/request_focus/m_has_focus |
| `NekoUI/NekoUI/Engine/Engine.hpp` | 修改 | 添加 focus_widget/focus_next/m_focused_widget |
| `NekoUI/NekoUI/Engine/Engine.cpp` | 修改 | msg_dispatch 键盘事件路由到焦点控件，Tab 键切换 |
| `NekoUI/NekoUI/Widget/Component/TextInput.hpp` | 新建 | 单行文本输入框 |
| `NekoUI/NekoUI/Widget/Component/Checkbox.hpp` | 新建 | 复选框 |
| `NekoUI/main.cpp` | 修改 | 添加 TextInput + Checkbox 演示 |
| `NekoUI/NekoUI.vcxproj` | 修改 | 添加 TextInput.hpp + Checkbox.hpp |

---

### Task 1: Widget 焦点系统

**Files:**
- Modify: `NekoUI/NekoUI/Widget/Widget.hpp`

**Interfaces:**
- Produces: `focusable()`, `on_focus_gained()`, `on_focus_lost()`, `request_focus()`, `has_focus()`, `m_has_focus` (protected, Engine is friend)

- [ ] **Step 1: 读取当前 Widget.hpp**

```bash
cat NekoUI/NekoUI/Widget/Widget.hpp
```

- [ ] **Step 2: 添加焦点接口到 Widget 类**

在 `public:` 块中添加：
```cpp
virtual auto focusable() const -> bool { return false; }
virtual auto on_focus_gained() -> void {}
virtual auto on_focus_lost() -> void {}

auto request_focus() -> void;       // 声明，实现调用 engine 的 focus_widget
[[nodiscard]] auto has_focus() const -> bool { return m_has_focus; }
```

在 `protected:` 中添加：
```cpp
bool m_has_focus = false;

friend class neko::engine::Engine;
```

在 `handle_event` 的 WM_LBUTTONDOWN case 中添加焦点请求：
```cpp
case WM_LBUTTONDOWN:
    if (hit_test(context.mouse)) {
        if (focusable()) request_focus();
        return true;
    }
    return false;
```

- [ ] **Step 3: 添加 `request_focus()` 的 inline 实现**

```cpp
inline auto Widget::request_focus() -> void {
    // request_focus 需要通知 Engine。
    // 但 Widget 不直接持有 Engine 引用。
    // 实现方式：通过 context 回调 或 在 Engine 的 msg_dispatch 中处理。
    // 方案：focus 请求在 handle_event 的 WM_LBUTTONDOWN 处理中，
    // Engine::msg_dispatch 在 handle_event 返回 true 后检查
    // 鼠标点击位置，自动 set focus。
    // （request_focus 的具体实现见 Task 2 Engine 改动）
}
```

**实际设计**：焦点不通过 `request_focus()` 直接设（Widget 没有 Engine 引用），而是：
- Engine 的 `msg_dispatch` 中，鼠标事件遍历根控件 → 某控件 `handle_event` 返回 true → 如果该控件 `focusable()`，Engine 自动设焦点

所以 `request_focus()` 实际上不需要做任何事情——Engine 在鼠标事件分发时自动设置焦点。但保留接口以备后续需要。

- [ ] **Step 4: 提交**

```bash
git add NekoUI/NekoUI/Widget/Widget.hpp
git commit -m "feat: Widget 焦点系统——focusable/on_focus_gained/on_focus_lost"
```

---

### Task 2: Engine 焦点管理

**Files:**
- Modify: `NekoUI/NekoUI/Engine/Engine.hpp`
- Modify: `NekoUI/NekoUI/Engine/Engine.cpp`

**Interfaces:**
- Consumes: Widget's `focusable()`, `on_focus_gained/lost()`, `m_has_focus`
- Produces: `Engine::focus_widget()`, `Engine::focused_widget()`, `Engine::focus_next()`, `m_focused_widget`

- [ ] **Step 1: 修改 `Engine.hpp` 添加焦点成员和方法**

```cpp
// 在 public: 中添加：
auto focus_widget(widget::Widget* w) -> void;
[[nodiscard]] auto focused_widget() const -> widget::Widget* { return m_focused_widget; }

// 在 private: 中添加：
auto focus_next() -> void;  // Tab 切换

// 在 private 成员中添加：
widget::Widget* m_focused_widget = nullptr;
```

- [ ] **Step 2: 实现 `focus_widget`**

在 `Engine.cpp` 中添加：

```cpp
auto Engine::focus_widget(widget::Widget* w) -> void {
    if (m_focused_widget == w) return;
    // 旧焦点失去焦点
    if (m_focused_widget) {
        m_focused_widget->m_has_focus = false;
        m_focused_widget->on_focus_lost();
    }
    // 新焦点获得焦点
    m_focused_widget = w;
    if (w) {
        w->m_has_focus = true;
        w->on_focus_gained();
    }
}
```

- [ ] **Step 3: 实现 `focus_next`（Tab 切换）**

```cpp
auto Engine::focus_next() -> void {
    // 收集所有 focusable 控件
    std::vector<widget::Widget*> focusable_widgets;
    for (auto& root : m_root_widgets) {
        collect_focusable(root.get(), focusable_widgets);
    }
    if (focusable_widgets.empty()) return;

    // 找当前焦点在列表中的位置，切换到下一个
    auto it = std::ranges::find(focusable_widgets, m_focused_widget);
    size_t idx = (it != focusable_widgets.end())
        ? (std::distance(focusable_widgets.begin(), it) + 1) % focusable_widgets.size()
        : 0;
    focus_widget(focusable_widgets[idx]);
}
```

需要辅助方法 `collect_focusable`：

```cpp
// 在 Engine 的匿名命名空间或私有方法中
static auto collect_focusable(widget::Widget* w, std::vector<widget::Widget*>& out) -> void {
    if (w->focusable()) out.push_back(w);
    for (auto* child : w->children()) {
        collect_focusable(child, out);
    }
}
```

- [ ] **Step 4: 修改 `msg_dispatch` 键盘路由**

将 `msg_dispatch` 的 default 分支改为：

```cpp
default:
    switch (msg) {
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
            for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
                if ((*it)->handle_event(context, msg, wparam, lparam)) {
                    // 如果是 LBUTTONDOWN，自动设焦点到命中的 focusable 控件
                    if (msg == WM_LBUTTONDOWN && (*it)->focusable()) {
                        focus_widget(it->get());
                    }
                    break;
                }
            }
            break;
        case WM_KEYDOWN:
            if (wparam == VK_TAB) {
                focus_next();
                return;
            }
            [[fallthrough]];
        case WM_KEYUP:
        case WM_CHAR:
            if (m_focused_widget) {
                m_focused_widget->handle_event(context, msg, wparam, lparam);
            }
            break;
        default:
            // 其他事件保持原有逻辑
            for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
                if ((*it)->handle_event(context, msg, wparam, lparam)) {
                    break;
                }
            }
            break;
    }
    break;
```

- [ ] **Step 5: 提交**

```bash
git add NekoUI/NekoUI/Engine/Engine.hpp NekoUI/NekoUI/Engine/Engine.cpp
git commit -m "feat: Engine 焦点管理——focus_widget/focus_next，键盘路由"
```

---

### Task 3: TextInput 组件

**Files:**
- Create: `NekoUI/NekoUI/Widget/Component/TextInput.hpp`

- [ ] **Step 1: 创建 TextInput.hpp**

```cpp
#pragma once

#include "../Widget.hpp"
#include <string>
#include <chrono>
#include <functional>

namespace neko::widget {

    class TextInput final : public Widget {
    public:
        explicit TextInput(glm::ivec4 bounds = {}, std::string placeholder = "")
            : m_placeholder(std::move(placeholder))
        {
            set_bounds(bounds);
        }

        auto update(engine::Context& context) -> void override {
            // 光标闪烁
            const auto now = std::chrono::steady_clock::now();
            if (now - m_cursor_tick > std::chrono::milliseconds(500)) {
                m_cursor_visible = !m_cursor_visible;
                m_cursor_tick = now;
                context.dirty = true;
            }
            Widget::update(context);
        }

        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            const auto& b = bounds();

            // 背景 + 边框
            backend.draw_rect_fill(b, m_bg_color);
            backend.draw_rect(b, has_focus() ? m_focus_border_color : m_border_color, 1);

            // 如果有选中，画选中背景
            if (has_focus() && m_sel_start >= 0 && m_sel_start != m_cursor_pos) {
                int sel_begin = (std::min)(m_sel_start, m_cursor_pos);
                int sel_end   = (std::max)(m_sel_start, m_cursor_pos);
                // 简化：全选高亮整个输入框背景
                backend.draw_rect_fill({b.x + 2, b.y + 2, b.z - 4, b.w - 4}, m_sel_color);
            }

            // 文字
            auto display_text = m_text.empty() ? m_placeholder : m_text;
            auto text_color = m_text.empty() ? m_placeholder_color : m_text_color;
            backend.draw_text(display_text, {b.x + 4, b.y + 4}, text_color);

            // 光标（有焦点时）
            if (has_focus() && m_cursor_visible) {
                // 简化：光标画在文字开始处右侧 4px
                int cx = b.x + 4 + m_cursor_pos * 8;  // 粗略估计每个字符 8px
                backend.draw_rect({cx, b.y + 3, 2, b.w - 6}, m_cursor_color);
            }

            Widget::draw(context, backend);
        }

        auto handle_event(engine::Context& context, UINT msg,
                          WPARAM wparam, LPARAM /*lparam*/) -> bool override
        {
            if (msg == WM_CHAR) {
                const auto ch = static_cast<char>(wparam);
                if (ch >= 32 && ch <= 126) {  // 可打印 ASCII
                    m_text.insert(m_cursor_pos, 1, ch);
                    ++m_cursor_pos;
                    m_sel_start = -1;
                    context.dirty = true;
                    if (on_text_changed) on_text_changed(m_text);
                }
                return true;
            }

            if (msg == WM_KEYDOWN) {
                switch (wparam) {
                    case VK_BACK:
                        if (m_cursor_pos > 0 && !m_text.empty()) {
                            m_text.erase(m_cursor_pos - 1, 1);
                            --m_cursor_pos;
                            m_sel_start = -1;
                            context.dirty = true;
                            if (on_text_changed) on_text_changed(m_text);
                        }
                        return true;
                    case VK_DELETE:
                        if (m_cursor_pos < static_cast<int>(m_text.size())) {
                            m_text.erase(m_cursor_pos, 1);
                            context.dirty = true;
                            if (on_text_changed) on_text_changed(m_text);
                        }
                        return true;
                    case VK_LEFT:
                        if (m_cursor_pos > 0) { --m_cursor_pos; m_sel_start = -1; context.dirty = true; }
                        return true;
                    case VK_RIGHT:
                        if (m_cursor_pos < static_cast<int>(m_text.size())) { ++m_cursor_pos; m_sel_start = -1; context.dirty = true; }
                        return true;
                    case VK_HOME:
                        m_cursor_pos = 0; m_sel_start = -1; context.dirty = true;
                        return true;
                    case VK_END:
                        m_cursor_pos = static_cast<int>(m_text.size()); m_sel_start = -1; context.dirty = true;
                        return true;
                    case 'A':
                        if (GetKeyState(VK_CONTROL) & 0x8000) {
                            m_sel_start = 0; m_cursor_pos = static_cast<int>(m_text.size());
                            context.dirty = true;
                            return true;
                        }
                        break;
                }
                return true;
            }

            return Widget::handle_event(context, msg, wparam, lparam);
        }

        auto focusable() const -> bool override { return true; }
        auto on_focus_gained() -> void override {
            m_cursor_visible = true;
            m_cursor_tick = std::chrono::steady_clock::now();
        }
        auto on_focus_lost() -> void override {
            m_cursor_visible = false;
            m_sel_start = -1;
        }

        auto text() const -> const std::string& { return m_text; }
        auto set_text(std::string_view t) -> void {
            m_text = t;
            m_cursor_pos = static_cast<int>(m_text.size());
            m_sel_start = -1;
        }
        auto set_placeholder(std::string_view t) -> void { m_placeholder = t; }

        std::function<void(std::string_view)> on_text_changed;

    private:
        std::string m_text;
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

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI/Widget/Component/TextInput.hpp
git commit -m "feat: 添加 TextInput 单行文本输入框组件"
```

---

### Task 4: Checkbox 组件

**Files:**
- Create: `NekoUI/NekoUI/Widget/Component/Checkbox.hpp`

- [ ] **Step 1: 创建 Checkbox.hpp**

```cpp
#pragma once

#include "../Widget.hpp"
#include "Animation.hpp"
#include <string>
#include <functional>

namespace neko::widget {

    class Checkbox final : public Widget {
    public:
        explicit Checkbox(glm::ivec4 bounds = {}, std::string label = "")
            : m_label(std::move(label))
        {
            set_bounds(bounds);
            m_anim = m_checked
                ? animation::EaseOutQuadAnimation<glm::vec4>{m_unchecked_color, m_checked_color, 150}
                : animation::EaseOutQuadAnimation<glm::vec4>{m_checked_color, m_unchecked_color, 150};
            m_anim.set_progress(1.0f); // 立刻到最终状态
        }

        auto update(engine::Context& context) -> void override {
            Widget::update(context);
            m_anim.update();
        }

        auto draw(engine::Context& context, backend::Backend& backend) -> void override {
            const auto& b = bounds();
            constexpr int box = 18;
            const int bx = b.x;
            const int by = b.y + (b.w - box) / 2;

            // 方框背景（动画色）
            glm::ivec4 box_color{
                static_cast<int>(m_anim()(0) * 255.0f + 0.5f),
                static_cast<int>(m_anim()(1) * 255.0f + 0.5f),
                static_cast<int>(m_anim()(2) * 255.0f + 0.5f),
                255,
            };

            // 方框背景填充
            if (m_checked || m_anim.progress() > 0.0f) {
                backend.draw_rect_fill({bx, by, box, box}, box_color);
            }

            // 方框边框
            auto border = has_focus() ? m_focus_border : m_normal_border;
            backend.draw_rect({bx, by, box, box}, border, 1);

            // 选中的勾号（简化：用填充色矩形表示）
            if (m_checked || m_anim.progress() > 0.0f) {
                // 勾号暂时用白色矩形近似，后续可改为实际勾号绘制
                backend.draw_rect_fill({bx + 4, by + 4, box - 8, box - 8}, m_check_color);
            }

            // 标签
            if (!m_label.empty()) {
                backend.draw_text(m_label, {bx + box + 8, b.y + 4}, m_text_color);
            }

            Widget::draw(context, backend);
        }

        auto handle_event(engine::Context& context, UINT msg,
                          WPARAM wparam, LPARAM lparam) -> bool override
        {
            if (msg == WM_LBUTTONDOWN) {
                if (hit_test(context.mouse)) {
                    toggle(context);
                    return true;
                }
                return false;
            }
            if (msg == WM_KEYDOWN && wparam == VK_SPACE) {
                toggle(context);
                return true;
            }
            return Widget::handle_event(context, msg, wparam, lparam);
        }

        auto focusable() const -> bool override { return true; }

        auto is_checked() const -> bool { return m_checked; }
        auto set_checked(bool checked) -> void {
            if (m_checked == checked) return;
            m_checked = checked;
            if (m_checked) {
                m_anim = animation::EaseOutQuadAnimation<glm::vec4>{m_unchecked_color, m_checked_color, 150};
            } else {
                m_anim = animation::EaseOutQuadAnimation<glm::vec4>{m_checked_color, m_unchecked_color, 150};
            }
        }
        auto toggle() -> void {
            set_checked(!m_checked);
            if (on_toggled) on_toggled(m_checked);
        }

        std::function<void(bool)> on_toggled;

    private:
        void toggle(engine::Context& context) {
            m_checked = !m_checked;
            if (m_checked) {
                m_anim = animation::EaseOutQuadAnimation<glm::vec4>{m_unchecked_color, m_checked_color, 150};
            } else {
                m_anim = animation::EaseOutQuadAnimation<glm::vec4>{m_checked_color, m_unchecked_color, 150};
            }
            context.dirty = true;
            if (on_toggled) on_toggled(m_checked);
        }

        bool m_checked = false;
        std::string m_label;
        animation::EaseOutQuadAnimation<glm::vec4> m_anim{
            {0.7f, 0.7f, 0.75f, 1.0f},  // unchecked
            {0.24f, 0.47f, 0.86f, 1.0f}, // checked
            150
        };

        glm::vec4 m_unchecked_color{0.7f, 0.7f, 0.75f, 1.0f};
        glm::vec4 m_checked_color{0.24f, 0.47f, 0.86f, 1.0f};
        type::Color m_check_color{255, 255, 255, 255};
        type::Color m_text_color{30, 30, 30, 255};
        type::Color m_normal_border{140, 140, 150, 255};
        type::Color m_focus_border{60, 120, 220, 255};
    };

} // namespace neko::widget
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI/Widget/Component/Checkbox.hpp
git commit -m "feat: 添加 Checkbox 复选框组件"
```

---

### Task 5: 更新 main.cpp 演示

**Files:**
- Modify: `NekoUI/main.cpp`

- [ ] **Step 1: 添加 include 和新控件**

添加：
```cpp
#include "NekoUI/Widget/Component/TextInput.hpp"
#include "NekoUI/Widget/Component/Checkbox.hpp"
```

在 Button 之后添加 TextInput 和 Checkbox：
```cpp
auto& input = engine.add<neko::widget::TextInput>(glm::ivec4{100, 170, 200, 30}, "请输入...");
input.on_text_changed = [](std::string_view text) -> void {
    std::println("[NekoUI] Text: {}", text);
};

auto& cb = engine.add<neko::widget::Checkbox>(glm::ivec4{100, 220, 200, 24}, "同意协议");
cb.on_toggled = [](bool checked) -> void {
    std::println("[NekoUI] Checkbox: {}", checked);
};
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/main.cpp
git commit -m "feat: main.cpp 添加 TextInput + Checkbox 演示"
```

---

### Task 6: vcxproj + 构建验证

**Files:**
- Modify: `NekoUI/NekoUI.vcxproj`

- [ ] **Step 1: 添加新组件到 vcxproj**

在 ClInclude 中添加：
```xml
<ClInclude Include="NekoUI\Widget\Component\TextInput.hpp" />
<ClInclude Include="NekoUI\Widget\Component\Checkbox.hpp" />
```

- [ ] **Step 2: 提交**

```bash
git add NekoUI/NekoUI.vcxproj
git commit -m "chore: vcxproj 添加 TextInput.hpp + Checkbox.hpp"
```

- [ ] **Step 3: 构建**

```bash
msbuild NekoUI.slnx /p:Configuration=Debug /p:Platform=x64
```

或用户指定构建命令。

- [ ] **Step 4: 修复编译错误后提交**
