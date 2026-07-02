#include "Widget.hpp"
neko::widget::Widget::~Widget() = default;

auto neko::widget::Widget::draw(engine::Context& context, backend::Backend& backend) -> void {
    for (auto* child : children_sorted_asc()) {
        child->draw(context, backend);
    }
    m_dirty = false;
}

auto neko::widget::Widget::layout(Constraints constraints) -> void {}

auto neko::widget::Widget::update(engine::Context& context) -> void {
    for (auto* child : m_children) {
        child->update(context);
    }
}

auto neko::widget::Widget::handle_event(engine::Context& context, const UINT msg, const WPARAM wparam, const LPARAM lparam) -> bool {
    for (auto* child : children_sorted_desc()) {
        if (child->handle_event(context, msg, wparam, lparam)) {
            return true;
        }
    }
    switch (msg) {
        case WM_LBUTTONDOWN:
            if (context.mouse.is_inside(m_bounds, context.dpi_scale)) {
                if (focusable()) {
                    if (context.request_focus) {
                        context.request_focus(this);
                    }
                }
                return true;
            }
            return false;
        case WM_MOUSEMOVE:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_MOUSEWHEEL:
            if (context.mouse.is_inside(m_bounds, context.dpi_scale)) {
                return true;
            }
            return false;
        default:
            return false;
    }
}

auto neko::widget::Widget::hit_test(const mouse::Mouse& mouse) const -> bool {
    if (!m_visible) {
        return false;
    }
    return mouse.is_inside(m_bounds);
}

auto neko::widget::Widget::mark_dirty() -> void {
    m_dirty = true;
    if (m_parent != nullptr) {
        m_parent->mark_dirty();
    }
}

auto neko::widget::Widget::dirty() const -> bool {
    return m_dirty;
}

auto neko::widget::Widget::clear_dirty() -> void {
    m_dirty = false;
}

auto neko::widget::Widget::children() const -> const std::vector<Widget*>& {
    return m_children;
}

auto neko::widget::Widget::parent() const -> Widget* {
    return m_parent;
}

auto neko::widget::Widget::child_count() const -> size_t {
    return m_children.size();
}

auto neko::widget::Widget::root() -> Widget* {
    auto* cur = this;
    while (cur->m_parent != nullptr) {
        cur = cur->m_parent;
    }
    return cur;
}

auto neko::widget::Widget::set_bounds(int x, int y, int w, int h) -> void {
    m_bounds = {x, y, w, h};
    mark_dirty();
}

auto neko::widget::Widget::set_bounds(const glm::ivec4 bounds) -> void {
    m_bounds = bounds;
    mark_dirty();
}

auto neko::widget::Widget::bounds() const -> const glm::ivec4& {
    return m_bounds;
}

auto neko::widget::Widget::x() const -> int {
    return m_bounds.x;
}

auto neko::widget::Widget::y() const -> int {
    return m_bounds.y;
}

auto neko::widget::Widget::width() const -> int {
    return m_bounds.z;
}

auto neko::widget::Widget::height() const -> int {
    return m_bounds.w;
}

auto neko::widget::Widget::set_z_order(const int order) -> void {
    m_z_order = order;
}

auto neko::widget::Widget::z_order() const -> int {
    return m_z_order;
}

auto neko::widget::Widget::set_visible(const bool v) -> void {
    m_visible = v;
    mark_dirty();
}

auto neko::widget::Widget::visible() const -> bool {
    return m_visible;
}

auto neko::widget::Widget::focusable() const -> bool {
    return false;
}

auto neko::widget::Widget::on_focus_gained() -> void {}
auto neko::widget::Widget::on_focus_lost() -> void {}

auto neko::widget::Widget::has_focus() const -> bool {
    return m_has_focus;
}

auto neko::widget::Widget::animate(const std::chrono::milliseconds dt) -> void {
    for (auto* child : m_children) {
        if (child->m_visible) {
            child->animate(dt);
        }
    }
}

neko::widget::Widget::Widget() {
    state::g_auto_bind_stack.push_back([this]() {
        mark_dirty();
        if (m_notify_rerender) {
            m_notify_rerender();
        }
    });
}

auto neko::widget::Widget::register_child(Widget* child) -> void {
    child->m_parent = this;
    m_children.push_back(child);
    mark_dirty();
}

auto neko::widget::Widget::unregister_child(Widget* child) -> void {
    const auto it = std::ranges::find(m_children, child);
    if (it != m_children.end()) {
        m_children.erase(it);
        child->m_parent = nullptr;
        mark_dirty();
    }
}

auto neko::widget::Widget::children_sorted_asc() const -> std::vector<Widget*> {
    auto sorted = m_children;
    std::ranges::sort(sorted, std::ranges::less{}, &Widget::z_order);
    return sorted;
}

auto neko::widget::Widget::children_sorted_desc() const -> std::vector<Widget*> {
    auto sorted = m_children;
    std::ranges::sort(sorted, std::ranges::greater{}, &Widget::z_order);
    return sorted;
}
