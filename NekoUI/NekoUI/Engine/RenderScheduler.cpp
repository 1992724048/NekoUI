// 2026-08-10 06:01:10

#include "RenderScheduler.hpp"
#include "InvalidationTracker.hpp"

#include <chrono>
#include <thread>

namespace neko::engine {
    RenderScheduler::RenderScheduler(FrameCallback callback, std::weak_ptr<InvalidationTracker> invalidation) :
        frame_callback_(std::move(callback)),
        invalidation_(std::move(invalidation)) {
        render_thread_ = std::jthread(&RenderScheduler::render_loop, this);
    }

    RenderScheduler::~RenderScheduler() {
        stop();
    }

    auto RenderScheduler::request_frame() -> void {
        pending_.store(true, std::memory_order_relaxed);
        render_notify_.notify_one();
    }

    auto RenderScheduler::set_pending_size(const int width, const int height) -> void {
        pending_width_.store(width, std::memory_order_relaxed);
        pending_height_.store(height, std::memory_order_relaxed);
        resize_pending_.store(true, std::memory_order_release);
    }

    auto RenderScheduler::stop() -> void {
        render_thread_.request_stop();
        render_notify_.notify_one();
        if (render_thread_.joinable() && render_thread_.get_id() != std::this_thread::get_id()) {
            render_thread_.join();
        }
    }

    auto RenderScheduler::consume_resize() -> std::optional<type::Vec2I> {
        if (resize_pending_.exchange(false, std::memory_order_acquire)) {
            return type::Vec2I{.x = pending_width_.load(std::memory_order_relaxed), .y = pending_height_.load(std::memory_order_relaxed)};
        }
        return std::nullopt;
    }

    auto RenderScheduler::pending_size() const -> type::Vec2I {
        return type::Vec2I{.x = pending_width_.load(std::memory_order_relaxed), .y = pending_height_.load(std::memory_order_relaxed)};
    }

    auto RenderScheduler::render_loop() -> void {
        while (!render_thread_.get_stop_token().stop_requested()) {
            if (!render_wait()) {
                break;
            }
            frame_callback_();
        }
    }

    auto RenderScheduler::render_wait() -> bool {
        const auto invalidation = invalidation_.lock();
        if (invalidation && invalidation->has_active_animations()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            return true;
        }
        std::unique_lock lock(render_mutex_);
        render_notify_.wait(lock,
                            [this, &invalidation] -> bool {
                                return render_thread_.get_stop_token().stop_requested() || pending_.load(std::memory_order_relaxed) || (invalidation && invalidation->needs_frame());
                            });
        pending_.store(false, std::memory_order_relaxed);
        return !render_thread_.get_stop_token().stop_requested();
    }
}
