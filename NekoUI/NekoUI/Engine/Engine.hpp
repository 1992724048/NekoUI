#pragma once
#include <array>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>

#include "Context.hpp"

#include "../Type.hpp"
#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    //! @brief 单窗口控件引擎
    class Engine final {
    public:
        explicit Engine(HWND hwnd);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;

        //! @brief 设置控件树
        auto set_widget(std::shared_ptr<widget::Widget> widget) -> void;

        //! @brief 触发重建
        auto rebuild() -> void;

        //! @brief 投递窗口消息到消息队列（满时阻塞等待）
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;
    private:
        static constexpr size_t MSG_QUEUE_MAX = 32;

        using MsgEvent = std::tuple<UINT, WPARAM, LPARAM>;

        auto render_loop() -> void;
        auto msg_loop() -> void;

        auto anim_inc() -> void;
        auto anim_dec() -> void;

        backend::Backend backend;
        Context context{};

        // 渲染线程
        std::mutex render_lock;
        std::condition_variable render_notify;
        std::jthread render_thread;

        // 消息队列/消息线程
        std::array<MsgEvent, MSG_QUEUE_MAX> msg_queue{};
        std::mutex msg_mutex;
        std::condition_variable msg_notify;
        std::condition_variable msg_space;
        std::jthread msg_thread;
        size_t msg_head{}, msg_tail{}, msg_count{};

        // 窗口大小调整
        std::atomic_bool resize_pending{false};
        glm::ivec2 resize_size{};

        std::atomic_int animation{};
        std::atomic_bool pending{};
        std::shared_ptr<widget::Widget> widget;
    };
} // namespace neko::engine
