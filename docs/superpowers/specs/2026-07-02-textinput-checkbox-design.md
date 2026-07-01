# TextInput + Checkbox 组件设计

> NekoUI — Win32 保留模式 UI 引擎
> 日期: 2026-07-02
> 状态: 已批准设计，待实现

## 概述

在保留模式控件系统基础上，添加焦点管理、TextInput（单行文本输入框）和 Checkbox（复选框）组件。

## 1. 焦点系统

### Widget 基类新增

```cpp
class Widget {
public:
    virtual auto focusable() const -> bool { return false; }
    virtual auto on_focus_gained() -> void {}
    virtual auto on_focus_lost() -> void {}

    auto request_focus() -> void;               // 通知引擎设此控件为焦点
    [[nodiscard]] auto has_focus() const -> bool { return m_has_focus; }

protected:
    bool m_has_focus = false;

    friend class neko::engine::Engine;  // Engine 设 m_has_focus
};
```

### `handle_event` 基类默认行为新增

鼠标左键按下时，如果命中控件且 `focusable() => true`，自动请求焦点：

```cpp
case WM_LBUTTONDOWN:
    if (hit_test(context.mouse)) {
        if (focusable()) request_focus();
        return true;
    }
    return false;
```

### Engine 新增

```cpp
class Engine {
public:
    auto focus_widget(Widget* w) -> void;
    [[nodiscard]] auto focused_widget() const -> Widget*;
    auto focus_next() -> void;  // Tab 切换

private:
    Widget* m_focused_widget = nullptr;
};
```

### `msg_dispatch` 键盘事件路由

键盘消息（WM_KEYDOWN / WM_KEYUP / WM_CHAR）直接路由到 `m_focused_widget`。鼠标消息保持原有遍历逻辑。

Tab 键在 Engine 级处理：`focus_next()` 按 z_order 顺序找下一个 `focusable()` 控件。

### 新增/修改文件

| 文件 | 操作 | 内容 |
|------|------|------|
| `Widget/Widget.hpp` | 修改 | 添加 `focusable()`, `on_focus_gained/lost()`, `request_focus()`, `m_has_focus` |
| `Engine/Engine.hpp` | 修改 | 添加 `focus_widget()`, `focused_widget()`, `focus_next()`, `m_focused_widget` |
| `Engine/Engine.cpp` | 修改 | `msg_dispatch` 键盘事件转发，Tab 键处理 |

## 2. TextInput 组件

### 新增文件

- `Widget/Component/TextInput.hpp`

### 接口

```cpp
class TextInput final : public Widget {
public:
    explicit TextInput(glm::ivec4 bounds = {}, std::string placeholder = "");

    auto update(Context& ctx) -> void override;
    auto draw(Context& ctx, Backend& bk) -> void override;
    auto handle_event(Context& ctx, UINT msg, WPARAM wp, LPARAM lp) -> bool override;
    auto focusable() const -> bool override { return true; }
    auto on_focus_gained() -> void override;
    auto on_focus_lost() -> void override;

    auto text() const -> const std::string&;
    auto set_text(std::string_view text) -> void;
    auto set_placeholder(std::string_view text) -> void;

    std::function<void(std::string_view)> on_text_changed;
};
```

### 键盘处理

| 操作 | 处理 |
|------|------|
| `WM_CHAR` 可打印字符 | 插入到光标位置 (`m_text.insert(m_cursor_pos, ch)`) |
| `VK_BACK` | 删除光标前字符 |
| `VK_DELETE` | 删除光标处字符 |
| `VK_LEFT` | 光标左移一格 |
| `VK_RIGHT` | 光标右移一格 |
| `VK_HOME` | 光标移到开头 |
| `VK_END` | 光标移到末尾 |
| Shift + 方向 | 扩展/收缩选中范围 |
| `Ctrl + A` | 全选 |

### 绘制

- 背景填充（`m_bg_color`）+ 边框
- 如果文字宽度 > 控件宽度 → `m_scroll_offset` 滚动
- 如果有选中 → 选中区域蓝色背景
- 如果焦点且光标闪烁计时器触发 → 绘制竖线光标
- 如果没有文字 → 灰色 placeholder 文字

### 成员变量

```cpp
std::string m_text;
std::string m_placeholder;
int m_cursor_pos = 0;
int m_sel_start = -1;
bool m_cursor_visible = true;
TimePoint m_cursor_tick;

// 颜色
type::Color m_bg_color{240, 240, 245, 255};
type::Color m_text_color{30, 30, 30, 255};
type::Color m_cursor_color{60, 60, 60, 255};
type::Color m_sel_color{180, 200, 240, 255};
type::Color m_placeholder_color{180, 180, 180, 255};
```

## 3. Checkbox 组件

### 新增文件

- `Widget/Component/Checkbox.hpp`

### 接口

```cpp
class Checkbox final : public Widget {
public:
    explicit Checkbox(glm::ivec4 bounds = {}, std::string label = "");

    auto update(Context& ctx) -> void override;
    auto draw(Context& ctx, Backend& bk) -> void override;
    auto handle_event(Context& ctx, UINT msg, WPARAM wp, LPARAM lp) -> bool override;
    auto focusable() const -> bool override { return true; }

    auto is_checked() const -> bool;
    auto set_checked(bool checked) -> void;
    auto toggle() -> void;

    std::function<void(bool)> on_toggled;

private:
    bool m_checked = false;
    std::string m_label;
    // 颜色动画：选中/未选中切换
    // 焦点框
};
```

### 交互

- 鼠标点击：`toggle()`
- 空格键（有焦点时）：`toggle()`
- `toggle()` 切换 `m_checked`，触发 `on_toggled` 回调

### 绘制

- 方框（18×18）：外边框 + 选中时内部填充 + 勾号（✓）
- 焦点时外框高亮
- 标签文字在方框右侧居中

## 边界情况

1. **TextInput 空文字**：显示 placeholder（灰色斜体）
2. **TextInput 文字过长**：`m_scroll_offset` 跟随光标自动滚动
3. **焦点切换**：先失去旧焦点 `on_focus_lost()`，再获得新焦点 `on_focus_gained()`
4. **无焦点时键盘消息**：如果 `m_focused_widget == nullptr`，键盘消息被忽略
5. **Tab 循环**：`focus_next()` 遍历所有根控件的所有子控件，只找 `focusable()` 的

## 实现顺序

1. Widget 焦点系统（focusable/on_focus_gained/on_focus_lost/request_focus/m_has_focus）
2. Engine 焦点管理（focus_widget/focus_next/m_focused_widget，msg_dispatch 键盘路由）
3. TextInput 组件
4. Checkbox 组件
5. main.cpp 添加 TextInput + Checkbox 演示
6. 构建验证
