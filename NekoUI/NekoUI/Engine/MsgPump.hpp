#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <semaphore>
#include <thread>
#include <tuple>
#include <Windows.h>

namespace neko::engine {
    class MsgPump {
    public:
        using MessageHandler = std::function<void(UINT, WPARAM, LPARAM)>;

        explicit MsgPump(MessageHandler handler);
        ~MsgPump();

        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;
        auto stop() -> void;

    private:
        auto msg_loop() -> void;
        auto msg_dequeue() -> std::optional<std::tuple<UINT, WPARAM, LPARAM>>;

        struct Msg {
            UINT msg;
            WPARAM wparam;
            LPARAM lparam;
        };

        static constexpr size_t kQueueSize = 32;
        std::array<Msg, kQueueSize> msg_queue_;
        std::mutex msg_mutex_;
        std::condition_variable msg_notify_;
        std::counting_semaphore<kQueueSize> msg_space_{kQueueSize};
        std::jthread msg_thread_;
        size_t head_ = 0, tail_ = 0, count_ = 0;
        std::atomic_bool running_{true};
        MessageHandler handler_;
    };
} // namespace neko::engine
