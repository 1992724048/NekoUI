#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <type_traits>
#include <vector>

#include "Context.hpp"

#include "../Type.hpp"
#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    class Engine final {
    public:
        explicit Engine(HWND hwnd);
        ~Engine();

        Engine(const Engine&) = delete;
        auto operator=(const Engine&) -> Engine& = delete;

        //! @brief 添加根控件
        template<typename T, typename... Args>
        auto add(Args&&... args) -> T& {
            static_assert(std::is_base_of_v<widget::Widget, T>, "T must derive from widget::Widget");
            auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
            ptr->set_z_order(static_cast<int>(m_root_widgets.size()));
            auto& ref = *ptr;
            m_root_widgets.push_back(std::move(ptr));
            rebuild();
            return ref;
        }

        //! @brief 触发重建
        auto rebuild() -> void;

        //! @brief 投递窗口消息到消息队列（满时阻塞等待）
        auto push_msg(UINT msg, WPARAM wparam, LPARAM lparam) -> void;

        //! @brief 请求焦点
        auto focus_widget(widget::Widget* w) -> void;

        //! @brief 获取当前焦点控件
        [[nodiscard]] auto focused_widget() const -> widget::Widget* {
            return m_focused_widget;
        }
    private:
        static constexpr size_t MSG_QUEUE_MAX = 32;

        using MsgEvent = std::tuple<UINT, WPARAM, LPARAM>;

        auto render_loop() -> void;
        auto render_wait() -> void;
        auto render_frame() -> void;

        auto msg_loop() -> void;
        auto msg_dequeue() -> std::optional<MsgEvent>;
        auto msg_dispatch(UINT msg, WPARAM wparam, LPARAM lparam) -> void;

        auto anim_inc() -> void;
        auto anim_dec() -> void;

        auto focus_next() -> void;
        static auto collect_focusable(widget::Widget* w, std::vector<widget::Widget*>& out) -> void;

        backend::Backend backend;
        Context context{};

        std::mutex render_lock;
        std::condition_variable render_notify;
        std::jthread render_thread;

        std::array<MsgEvent, MSG_QUEUE_MAX> msg_queue{};
        std::mutex msg_mutex;
        std::condition_variable msg_notify;
        std::condition_variable msg_space;
        std::jthread msg_thread;
        size_t msg_head{}, msg_tail{}, msg_count{};

        std::atomic_bool resize_pending{false};
        glm::ivec2 resize_size{};

        std::atomic_int animation{};
        using Clock = std::chrono::steady_clock;
        Clock::time_point m_last_frame{Clock::now()};
        std::atomic_bool pending{};
        widget::Widget* m_focused_widget = nullptr;
        std::vector<std::unique_ptr<widget::Widget>> m_root_widgets{};
    };
} // namespace neko::engine
