#pragma once
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

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
        auto set_widget_tree(std::shared_ptr<widget::Widget> tree) -> void;
        //! @brief 触发重建
        auto rebuild() -> void;
    private:
        auto render_thread() -> void;

        auto anim_inc() -> void;
        auto anim_dec() -> void;

        backend::Backend backend;
        Context context{};

        std::mutex render_lock;
        std::condition_variable render_notify;
        std::jthread render_thread_;

        std::atomic_int animation{};
        std::shared_ptr<widget::Widget> widget;
    };
} // namespace neko::engine
