#pragma once
#include <atomic>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    class InvalidationTracker {
    public:
        auto mark_dirty() -> void;
        auto mark_widget_dirty(const std::weak_ptr<widget::Widget>& widget) -> void;
        auto anim_inc() -> void;
        auto anim_dec() -> void;

        auto is_dirty() const -> bool;
        auto has_active_animations() const -> bool;
        auto needs_frame() const -> bool;
        auto consume_dirty_list() -> std::vector<std::weak_ptr<widget::Widget>>;
        auto clear() -> void;
    private:
        std::atomic_bool dirty_{true};
        std::atomic_int animation_{0};
        std::shared_mutex dirty_list_mutex_;
        std::vector<std::weak_ptr<widget::Widget>> dirty_list_;
    };
} // namespace neko::engine
