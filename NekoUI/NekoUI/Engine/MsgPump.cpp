#include "MsgPump.hpp"

namespace neko::engine {
    MsgPump::MsgPump(MessageHandler handler) :
        handler_(std::move(handler)) {
        msg_thread_ = std::jthread(&MsgPump::msg_loop, this);
    }

    MsgPump::~MsgPump() {
        stop();
    }

    auto MsgPump::push_msg(const platform::Event& event) -> void {
        msg_space_.acquire();
        {
            std::scoped_lock lock(msg_mutex_);
            msg_queue_[tail_] = event;
            tail_ = (tail_ + 1) % kQueueSize;
            ++count_;
        }
        msg_notify_.notify_one();
    }

    auto MsgPump::request_stop() -> void {
        running_.store(false);
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
            auto ev = msg_dequeue();
            if (!ev.has_value()) {
                break;
            }
            handler_(*ev);
        }
    }

    auto MsgPump::msg_dequeue() -> std::optional<platform::Event> {
        std::unique_lock lock(msg_mutex_);
        msg_notify_.wait(lock,
                         [this] -> bool {
                             return !running_.load() || count_ > 0;
                         });
        if (!running_.load()) {
            return std::nullopt;
        }
        auto ev = msg_queue_[head_];
        head_ = (head_ + 1) % kQueueSize;
        --count_;
        lock.unlock();
        msg_space_.release();
        return ev;
    }
}
