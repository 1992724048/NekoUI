#include "InvalidationTracker.hpp"

namespace neko::engine {
    auto InvalidationTracker::mark_dirty() -> void {
        dirty_ = true;
    }

    auto InvalidationTracker::mark_widget_dirty(const std::weak_ptr<widget::Widget>& widget) -> void {
        std::unique_lock _(dirty_list_mutex_);
        dirty_list_.push_back(widget);
    }

    auto InvalidationTracker::anim_inc() -> void {
        ++animation_;
    }

    auto InvalidationTracker::anim_dec() -> void {
        --animation_;
    }

    auto InvalidationTracker::is_dirty() const -> bool {
        return dirty_;
    }

    auto InvalidationTracker::has_active_animations() const -> bool {
        return animation_ != 0;
    }

    auto InvalidationTracker::needs_frame() const -> bool {
        return dirty_ || !dirty_list_.empty() || animation_ != 0;
    }

    auto InvalidationTracker::consume_dirty_list() -> std::vector<std::weak_ptr<widget::Widget>> {
        std::unique_lock _(dirty_list_mutex_);
        auto list = std::move(dirty_list_);
        dirty_list_.clear();
        return list;
    }

    auto InvalidationTracker::clear() -> void {
        dirty_ = false;
        std::unique_lock _(dirty_list_mutex_);
        dirty_list_.clear();
    }
} // namespace neko::engine
