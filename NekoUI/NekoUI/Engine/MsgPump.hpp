#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <semaphore>
#include <thread>

#include "../Platform/Event.hpp"

namespace neko::engine {
    class MsgPump {
    public:
        using MessageHandler = std::function<void(platform::Event)>;

        explicit MsgPump(MessageHandler handler);
        ~MsgPump();

        auto push_msg(const platform::Event& event) -> void;
        auto request_stop() -> void;
        auto stop() -> void;
    private:
        auto msg_loop() -> void;
        auto msg_dequeue() -> std::optional<platform::Event>;

        static constexpr size_t kQueueSize = 32;
        std::array<platform::Event, kQueueSize> msg_queue_{};
        std::mutex msg_mutex_;
        std::condition_variable msg_notify_;
        std::counting_semaphore<kQueueSize> msg_space_{kQueueSize};
        std::jthread msg_thread_;
        size_t head_ = 0, tail_ = 0, count_ = 0;
        std::atomic_bool running_{true};
        MessageHandler handler_;
    };
}
