#pragma once
#include <Windows.h>
#include <array>
#include <condition_variable>
#include <optional>
#include <thread>

namespace neko::engine {
    class MsgLoop {
    public:
        using MsgEvent = std::tuple<UINT, WPARAM, LPARAM>;
        static constexpr size_t MSG_QUEUE_MAX = 32;

        // 投递窗口消息
        virtual auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void = 0;
    protected:
        std::array<MsgEvent, MSG_QUEUE_MAX> msg_queue{};
        std::mutex msg_mutex;
        std::condition_variable msg_notify;
        std::condition_variable msg_space;
        std::jthread msg_thread;
        size_t msg_head{}, msg_tail{}, msg_count{};

        virtual auto msg_loop() -> void = 0;
        virtual auto msg_dequeue() -> std::optional<MsgEvent> = 0;
        virtual auto msg_dispatch(UINT msg, WPARAM wparam, LPARAM lparam) -> void = 0;
    };
}
