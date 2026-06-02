#pragma once
#include <memory>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <string>
#include <unordered_map>

#include "../Type.hpp"
#include "../Widget/Widget.hpp"

#include "../Backend/Backend.hpp"
#include "../Window/Window.hpp"

namespace neko::engine {
    using namespace neko::type;
    using namespace neko::widget;
    using namespace neko::window;

    template<typename T> concept BackendType = requires(std::shared_ptr<T> ptr) {
        requires std::is_base_of_v<backend::Backend, T>; { ptr->get_window_handle() } -> std::same_as<Handle>; {
            ptr->render_callback
        } -> std::convertible_to<std::function<void()>>;
    };

    template<typename T> concept WindowsType = requires(std::shared_ptr<T> ptr) {
        requires std::is_base_of_v<Window, T>; { ptr->get_handle() } -> std::same_as<Handle>; {
            ptr->msg_callback
        } -> std::convertible_to<std::function<MsgResult()>>;
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

            std::unique_lock _(mutex);
            auto& ins = backend_windows[handle];
            ins.window = window;
            ins.backend = backend;

            window->msg_callback = std::bind(&Engine::msg_callback, this, std::ref(ins), std::ref(window->state));
            backend->render_callback = std::bind(&Engine::render_process, std::ref(ins));
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

        /**
         * <summary>
         * 销毁引擎资源
         * </summary>
         */
        auto destroy() -> void;

        /**
         * <summary>
         * 通过窗口与控件ID获取控件对象
         * </summary>
         * <param name="handle">窗口句柄</param>
         * <param name="widget_id">控件ID</param>
         * <returns>控件对象</returns>
         */
        auto get_widget(Handle handle, const std::string& widget_id) -> std::shared_ptr<Widget>;

        /**
         * <summary>
         * 设置控件树
         * </summary>
         * <param name="handle">窗口句柄</param>
         * <param name="widget_tree">控件树对象</param>
         * <returns>是否成功</returns>
         */
        auto set_widget_tree(Handle handle, const std::shared_ptr<Widget>& widget_tree) -> bool;
    private:
        struct ChildWindow {
            std::shared_ptr<backend::Backend> backend;
            std::shared_ptr<Window> window;

            std::mutex render_lock;
            std::condition_variable render_notify;
            std::jthread render_thread;

            std::atomic_bool init{false};
            std::atomic_bool dirty;
            std::atomic_int16_t animation;
            std::atomic<std::shared_ptr<Widget>> widget_tree;
        };

        auto msg_callback(ChildWindow& child, InputState& state) -> MsgResult;
        static auto render_thread(const std::stop_token& token, ChildWindow& window) -> void;
        static auto render_process(ChildWindow& window) -> void;
        static auto msg_process(ChildWindow& window) -> MsgResult;

        std::shared_mutex mutex;
        std::unordered_map<Handle, ChildWindow> backend_windows;
    };
} // namespace neko::engine
