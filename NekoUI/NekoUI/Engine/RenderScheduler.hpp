#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

#include "../Type.hpp"

namespace neko::engine {
    class InvalidationTracker;

    class RenderScheduler {
    public:
        using FrameCallback = std::function<void()>;

        RenderScheduler(FrameCallback callback, InvalidationTracker& invalidation);
        ~RenderScheduler();

        RenderScheduler(const RenderScheduler&) = delete;
        auto operator=(const RenderScheduler&) -> RenderScheduler& = delete;
        RenderScheduler(RenderScheduler&&) = delete;
        auto operator=(RenderScheduler&&) -> RenderScheduler& = delete;

        auto request_frame() -> void;
        auto set_pending_size(int width, int height) -> void;
        auto stop() -> void;

        auto consume_resize() -> std::optional<type::Vec2I>;
        auto pending_size() const -> type::Vec2I;
    private:
        auto render_loop() -> void;
        auto render_wait() -> bool;

        std::jthread render_thread_;
        std::condition_variable render_notify_;
        std::mutex render_mutex_;
        std::atomic_bool pending_{false};
        std::atomic_bool resize_pending_{false};
        std::atomic<int> pending_width_{0};
        std::atomic<int> pending_height_{0};
        FrameCallback frame_callback_;
        InvalidationTracker& invalidation_;
    };
} // namespace neko::engine
