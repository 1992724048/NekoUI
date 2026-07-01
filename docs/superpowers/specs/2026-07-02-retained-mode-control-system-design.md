# 保留模式控件系统 — 设计文档

> NekoUI — Win32 即时模式 UI 引擎  
> 日期: 2026-07-02  
> 状态: 已批准设计，待实现

## 概述

将 NekoUI 从单 Widget 的即时模式架构重构为**保留模式控件系统**，支持：
- 扁平注册（引擎持有多个根控件，无手动树管理）
- 自动子控件遍历（`Sub<T>` 包装器自动注册，系统自动递归绘制/更新/事件）
- 点定位 + 自动布局容器混用
- Z-Order 排序与事件路由
- 最小化使用者认知负担

## 架构

### Widget 基类 (`Widget/Widget.hpp`)

```cpp
namespace neko::widget {

struct Constraints {
    int min_w = 0;
    int max_w = INT_MAX;
    int min_h = 0;
    int max_h = INT_MAX;
};

class Widget {
public:
    // === 生命周期 ===
    virtual auto update(Context& ctx) -> void;
    virtual auto layout(Constraints constraints) -> void;
    virtual auto draw(Context& ctx, Backend& backend) -> void;
    virtual auto handle_event(Context& ctx, uint32_t msg,
                              uint64_t wparam, int64_t lparam) -> bool;

    // === 命中测试 ===
    virtual auto hit_test(const neko::mouse::Mouse& mouse) const -> bool;

    // === Dirty 管理 ===
    auto mark_dirty() -> void;
    auto dirty() const -> bool;
    auto clear_dirty() -> void;

    // === 树结构（只读） ===
    auto children() const -> const std::vector<Widget*>&;
    auto parent() const -> Widget*;
    auto child_count() const -> size_t;
    auto root() const -> Widget*;

    // === 坐标与尺寸 ===
    auto set_bounds(int x, int y, int w, int h) -> void;
    auto bounds() const -> const glm::ivec4&;
    auto x() const -> int;
    auto y() const -> int;
    auto width() const -> int;
    auto height() const -> int;
    auto set_z_order(int order) -> void;
    auto z_order() const -> int;

    // === 可见性 ===
    auto set_visible(bool v) -> void;
    auto visible() const -> bool;

protected:
    Widget() = default;
    ~Widget() = default;  // 由 unique_ptr 或成员生命周期管理

private:
    template<typename T> friend class Sub;
    auto register_child(Widget* child) -> void;
    auto unregister_child(Widget* child) -> void;

    Widget* m_parent = nullptr;
    std::vector<Widget*> m_children;
    glm::ivec4 m_bounds{};  // x, y, width, height
    int m_z_order = 0;
    bool m_dirty = true;
    bool m_visible = true;
};

} // namespace neko::widget
```

关键设计决策：
- `m_children` 存裸指针（非拥有）— 子控件生命周期由引擎或所属类管理
- `add_child`/`remove_child` 是私有方法（通过 `Sub<T>` 友元类调用）
- `child_count()` 是普通方法（返回 `m_children.size()`），不再需要子类覆盖
- 基类 `draw()` 默认遍历子控件按 z_order 升序绘制
- 基类 `update()` 默认遍历子控件
- 基类 `handle_event()` 默认从高 z_order 子控件开始遍历
- 基类 `layout()` 默认空实现（子类按需覆盖）
- 基类 `hit_test()` 默认矩形命中测试
- `mark_dirty()` 向上传播到父控件
- 虚构函数非虚 — 生命周期由持有者管理（`unique_ptr` 或成员变量）

### Sub\<T\> 自动注册包装器 (`Widget/Sub.hpp`)

```cpp
namespace neko::widget {

template<typename T>
class Sub : public T {
    static_assert(std::is_base_of_v<Widget, T>);
public:
    template<typename... Args>
    explicit Sub(Widget* parent, Args&&... args)
        : T(std::forward<Args>(args)...)
    {
        if (parent) {
            Widget::register_child(this);
        }
    }

    ~Sub() {
        if (this->m_parent) {
            Widget::unregister_child(this);
        }
    }

    Sub(const Sub&) = delete;
    auto operator=(const Sub&) = delete;
    Sub(Sub&&) = default;
    auto operator=(Sub&&) -> Sub& = default;
};

} // namespace neko::widget
```

关键设计决策：
- 继承自 `T`（完美转发 Widget 功能）
- 构造时自动 `register_child`（零样板代码）
- 析构时自动 `unregister_child`
- 禁止拷贝，允许移动

### 引擎层改动 (`Engine/Engine.hpp`, `Engine/Engine.cpp`)

```cpp
class Engine {
public:
    // 添加根控件
    template<typename T, typename... Args>
    auto add(Args&&... args) -> T& {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        ptr->set_z_order(m_next_z++);
        auto& ref = *ptr;
        m_root_widgets.push_back(std::move(ptr));
        return ref;
    }

    auto remove(Widget& widget) -> void;
    auto find(std::string_view id) -> Widget*;
    // 可选 ID 支持：add() 可接受 id 参数，存入 map

private:
    std::vector<std::unique_ptr<Widget>> m_root_widgets;
    int m_next_z = 0;
    // 原有成员：消息队列、jthread、Context 等保持不变
};
```

渲染循环改动：
```cpp
void Engine::render_frame() {
    if (m_resize_pending) { backend.resize(...); }
    backend.begin();
    for (auto& root : m_root_widgets) {
        root->layout(m_current_constraints);
        root->draw(m_ctx, m_backend);
    }
    backend.end();
}
```

消息循环改动：
```cpp
void Engine::msg_dispatch(Context& ctx, uint32_t msg,
                          uint64_t wparam, int64_t lparam) {
    ctx.mouse.handle(msg, wparam, lparam);
    ctx.keyboard.handle(msg, wparam, lparam);

    // 事件从高 z_order 根控件开始分发
    for (auto it = m_root_widgets.rbegin(); it != m_root_widgets.rend(); ++it) {
        if ((*it)->handle_event(ctx, msg, wparam, lparam)) {
            break;
        }
    }

    if (msg == WM_SIZE) { m_resize_pending = true; ctx.dirty = true; }

    // 全树 update
    for (auto& root : m_root_widgets) {
        root->update(ctx);
    }
}
```

### Z-Order 与事件路由

- **绘制顺序**: z_order 从小到大（背景 → 前景）
- **事件命中**: z_order 从大到小（最上层优先）
- **默认分配**: `engine.add()` 按添加顺序递增 z_order
- 子控件列表的排序在 draw/handle_event 时按需排序（不持久排序，避免副作用）

### 布局系统

- **点定位**: 基类提供 `set_bounds(x, y, w, h)`，父控件在 `layout()` 中手动定位子控件
- **自动布局容器**: 内置 `VStack`（垂直排列）、`HStack`（水平排列）、`ZStack`（层叠排列），作为普通 Widget 的子类，有自己的 `layout()` 实现
- **Constraints**: 父控件传递给子控件的布局约束（最小/最大宽高），默认不限制

### Dirty 传播

- `mark_dirty()` 沿父指针向上传播到根控件
- 根控件 dirty → 通过 `context.dirty` 通知渲染线程
- `draw()` 绘制完成后 `clear_dirty()`
- 后续优化：跳过不可见或无动画且未脏的子树

### 现有 Button 迁移

- 删除 `child_count()` 和 `dirty()` 纯虚函数覆盖（由基类管理）
- `handle_event()` 改为先调基类版本（处理子控件事件），再处理自己的鼠标事件
- `update()` 和 `draw()` 保持现有逻辑不变
- `layout()` 如果不需要覆盖子控件位置，可省略

## 开发者用法

### 定义叶子控件

```cpp
class MyButton : public neko::widget::Widget {
    std::string m_label;
    // 完全自包含：draw、update、handle_event 全部自己实现
    auto draw(Context& ctx, Backend& bk) -> void override { /* ... */ }
    auto handle_event(...) -> bool override { /* 点击逻辑 */ }
};
```

### 组合控件

```cpp
class LoginPanel : public neko::widget::Widget {
    Sub<TextInput> m_user{this, "用户名"};
    Sub<TextInput> m_pass{this, "密码"};
    Sub<MyButton> m_btn{this, "登录"};

    auto layout(Constraints c) -> void override {
        set_bounds(0, 0, 250, 140);
        m_user.set_bounds(25, 20, 200, 30);
        m_pass.set_bounds(25, 60, 200, 30);
        m_btn.set_bounds(75, 100, 100, 30);
    }
};
```

### 注册到引擎

```cpp
auto engine = neko::engine::Engine{/* hwnd */};
engine.add<LoginPanel>();
engine.add<StatusBar>();
engine.add<NavigationBar>();
```

### 自动布局容器

```cpp
auto& toolbar = engine.add<neko::widget::layout::HStack>();
toolbar.set_bounds(0, 0, 800, 40);
// HStack 的 layout() 自动水平排列子控件
```

## 组件清单

### 新增文件
| 文件 | 内容 |
|------|------|
| `Widget/Sub.hpp` | `Sub<T>` 自动注册包装器 |
| `Widget/layout/Stack.hpp` | `VStack`、`HStack`、`ZStack` 布局容器 |

### 修改文件
| 文件 | 改动 |
|------|------|
| `Widget/Widget.hpp` | 添加 m_children/m_parent/m_bounds/m_z_order/m_dirty/m_visible，添加 register_child/unregister_child 私有方法，添加公开查询方法，修改虚函数默认实现 |
| `Engine/Engine.hpp` | 添加 m_root_widgets/m_next_z，添加 add\<T\>()/remove()/find() 模板方法 |
| `Engine/Engine.cpp` | render_frame/msg_dispatch 改为遍历根控件 |
| `Widget/Component/Button.hpp` | 删除 child_count/dirty 覆盖，适配新基类接口 |

### 删除的接口
- `Widget::child_count()` 纯虚函数 → 改为普通方法
- `Widget::dirty()` 纯虚函数 → 改为普通方法

## 边界情况与注意事项

1. **悬空指针**: `Sub<T>` 析构时 `unregister_child` 会擦除指针，而父控件可能先于子控件析构。C++ 成员析构顺序保证：子控件（成员）先于父控件（基类）析构，所以 `unregister_child` 执行时父控件 Widget 仍存活。安全。

2. **引擎析构**: `m_root_widgets` 是 `unique_ptr` 容器，析构时自动释放所有根控件。根控件的 `Sub<T>` 成员在根控件之前析构，自动取消注册。安全。

3. **事件竞争**: 消息线程和渲染线程共享 `Context`。`handle_event()` 在消息线程中调用，`draw()` 在渲染线程中调用。已有 `atomic_bool` 机制不变。

4. **空子控件**: `m_children` 为空时遍历安全。

5. **循环注册**: 目前不允许子控件拥有自己的父控件作为子控件（无向图检查可在后续版本添加）。

## 未解决的问题（Post-MVP）

- [ ] **自动布局容器策略**：VStack/HStack 的具体布局参数（对齐、填充、权重）
- [ ] **ID 查找**：`engine.find(id)` 实现（可选的，方便调试和事件通信）
- [ ] **裁剪（Clipping）**：子控件超出父 bounds 时是否裁剪
- [ ] **焦点管理**：键盘焦点的 Tab 顺序
- [ ] **更细粒度的脏区域**：只重绘脏区域而非全帧

## 实现顺序

1. Widget 基类改造（添加树结构、修改默认实现）
2. Sub\<T\> 包装器
3. 引擎层改动（根控件列表、遍历逻辑）
4. Button 迁移适配
5. 布局容器（VStack/HStack/ZStack）
6. 编译验证与测试
