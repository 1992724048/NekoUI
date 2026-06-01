#pragma once
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "../Type.hpp"
#include "../Widget/Widget.hpp"

#include "../Backend/Backend.hpp"
#include "../Window/Window.hpp"

namespace neko::engine {
    using namespace neko::type;

    template<typename T> concept BackendType = requires(T ptr) {
        requires std::is_base_of_v<backend::Backend, T>; { ptr.get_window_handle() } -> std::same_as<Handle>;
    };

    template<typename T> concept WindowsType = requires(std::shared_ptr<T> ptr) {
        requires std::is_base_of_v<window::Window, T>; { ptr->get_handle() } -> std::same_as<Handle>; {
            ptr->msg_callback
        } -> std::convertible_to<std::function<window::MsgResult()>>;
    };

    /**
     * <summary>
     * 控件处理引擎
     * </summary>
     */
    class Engine {
    public:
        Engine() = default;
        ~Engine() = default;

        /**
         * <summary>
         * 添加窗口
         * </summary>
         * <param name="window">窗口对象</param>
         * <param name="backend">后端对象</param>
         * <returns>结果</returns>
         */
        template<WindowsType W, BackendType B>
        auto add(std::shared_ptr<W> window, std::shared_ptr<B> backend) -> bool {
            if (window == nullptr || backend == nullptr) {
                return false;
            }
            const Handle window_handle = window->get_handle();
            const Handle handle = backend->get_window_handle();
            if (window_handle == nullptr || handle == nullptr || window_handle != handle) {
                return false;
            }
            window->msg_callback = std::bind(&Engine::msg_callback, this, window_handle, std::ref(window->state));

            std::unique_lock _(mutex);
            backend_windows[window_handle].window = window;
            backend_windows[handle].backend = backend;
            return true;
        }

        /**
         * <summary>
         * 移除窗口
         * </summary>
         * <param name="handle">窗口句柄</param>
         * <returns>结果</returns>
         */
        auto remove(Handle handle) -> bool;

        /**
         * <summary>
         * 是否可用
         * </summary>
         * <returns>结果</returns>
         */
        [[nodiscard]] auto is_ready() -> bool;
    private:
        struct ChildWindow {
            std::shared_ptr<backend::Backend> backend;
            std::shared_ptr<window::Window> window;

            std::shared_mutex mutex;
            std::unordered_map<std::string, std::shared_ptr<widget::Widget>> id2widget;

            std::mutex lock;
            std::condition_variable notification;

            std::jthread render_thread;

            std::atomic_bool init{false};
            std::atomic_bool dirty;
            std::atomic_int16_t animation;
            std::atomic<std::shared_ptr<widget::Widget>> widget_tree;
        };

        auto msg_callback(Handle handle, window::InputState& state) -> window::MsgResult;
        static auto render_thread(const std::stop_token& token, ChildWindow& window) -> void;
        static auto ui_process(ChildWindow& window) -> void;

        std::shared_mutex mutex;
        std::unordered_map<Handle, ChildWindow> backend_windows;
    };
} // namespace neko::engine
