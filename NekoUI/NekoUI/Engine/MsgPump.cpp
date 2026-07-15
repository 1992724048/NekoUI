#include "MsgPump.hpp"

namespace neko::engine {
    MsgPump::MsgPump(MessageHandler handler) : handler_(std::move(handler)) {
        msg_thread_ = std::jthread(&MsgPump::msg_loop, this);
    }

    MsgPump::~MsgPump() {
        stop();
    }

    auto MsgPump::push_msg(const UINT msg, const WPARAM wparam, const LPARAM lparam) -> void {
        msg_space_.acquire();
        {
            std::lock_guard lock(msg_mutex_);
            msg_queue_[tail_] = Msg{msg, wparam, lparam};
            tail_ = (tail_ + 1) % kQueueSize;
            ++count_;
        }
        msg_notify_.notify_one();
    }

    auto MsgPump::stop() -> void {
        running_.store(false);
        msg_notify_.notify_one();
        if (msg_thread_.joinable()) {
            msg_thread_.join();
        }
    }

    auto MsgPump::msg_loop() -> void {
        while (running_.load()) {
            const auto ev = msg_dequeue();
            if (!ev.has_value()) {
                break;
            }
            const auto [msg, wparam, lparam] = *ev;
            handler_(msg, wparam, lparam);
        }
    }

    auto MsgPump::msg_dequeue() -> std::optional<std::tuple<UINT, WPARAM, LPARAM>> {
        std::unique_lock lock(msg_mutex_);
        msg_notify_.wait(lock, [this] { return !running_.load() || count_ > 0; });
        if (!running_.load()) {
            return std::nullopt;
        }
        const auto ev = msg_queue_[head_];
        head_ = (head_ + 1) % kQueueSize;
        --count_;
        lock.unlock();
        msg_space_.release();
        return std::make_tuple(ev.msg, ev.wparam, ev.lparam);
    }
} // namespace neko::engine
