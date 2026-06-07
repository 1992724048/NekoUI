#pragma once
#include <memory>
#include <shared_mutex>
#include <string>
#include <thread>
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

    //! @brief 控件处理引擎
    class Engine {
    public:
        Engine() = default;
        ~Engine() = default;

        //! @brief 添加窗口
        //! @tparam W 约束
        //! @tparam B 约束
        //! @param window 窗口对象
        //! @param backend 后端对象
        //! @return 结果
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
            backend->render_callback = std::bind(render_process, std::ref(ins));
            ins.context.rebuild = std::bind(rebuild, std::ref(ins));
            ins.context.animation_start = std::bind(animation_count, std::ref(ins), 1);
            ins.context.animation_end = std::bind(animation_count, std::ref(ins), -1);

            rebuild(ins);
            return true;
        }

        //! @brief 移除窗口
        //! @param handle 窗口句柄
        //! @return true: 成功 \n false: 失败
        auto remove(Handle handle) -> bool;

        //! @brief 是否可用
        //! @return true: 可用 \n false: 不可用
        [[nodiscard]] auto is_ready() -> bool;

        //! @brief 销毁引擎资源
        auto destroy() -> void;

        //! @brief 设置控件树
        //! @param handle 窗口句柄
        //! @param widget_tree 控件树对象
        //! @return true: 成功 \n false: 失败
        auto set_widget_tree(Handle handle, const std::shared_ptr<Widget>& widget_tree) -> bool;
    private:
        struct ChildWindow {
            std::shared_ptr<backend::Backend> backend;
            std::shared_ptr<Window> window;

            std::mutex keys_lock;
            std::unordered_map<std::string, std::shared_ptr<Widget>> key2widget;

            std::mutex stack_lock;
            std::vector<std::shared_ptr<Widget>> widget_stack;

            std::mutex event_lock;
            std::vector<std::shared_ptr<Widget>> event_stack;

            std::mutex render_lock;
            std::condition_variable render_notify;
            std::jthread render_thread;

            Context context{};

            std::atomic_bool init{false};
            std::atomic_uint16_t animation;

            std::atomic<std::shared_ptr<Widget>> widget_tree;
        };

        auto msg_callback(ChildWindow& child, InputState& state) -> MsgResult;
        static auto render_thread(const std::stop_token& token, ChildWindow& window) -> void;
        static auto render_process(ChildWindow& window) -> void;
        static auto msg_process(ChildWindow& window, InputState& state) -> MsgResult;
        static auto rebuild(ChildWindow& window) -> void;
        static auto animation_count(ChildWindow& window, std::uint16_t num) -> void;
        static auto rerender(ChildWindow& window) -> void;

        std::shared_mutex mutex;
        std::unordered_map<Handle, ChildWindow> backend_windows;
    };
} // namespace neko::engine
