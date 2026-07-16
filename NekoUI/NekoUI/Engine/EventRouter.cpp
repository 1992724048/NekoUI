#include "EventRouter.hpp"

#include "Context.hpp"
#include "InvalidationTracker.hpp"
#include "MsgPump.hpp"
#include "RenderScheduler.hpp"
#include "WidgetTree.hpp"

#include "../Backend/Backend.hpp"
#include "../Widget/Widget.hpp"

namespace neko::engine {
    EventRouter::EventRouter(WidgetTree& tree,
                             device::Mouse& mouse,
                             device::Keyboard& keyboard,
                             Context& context,
                             backend::Backend& backend,
                             const std::shared_ptr<RenderScheduler>& scheduler,
                             const std::shared_ptr<MsgPump>& msg_pump,
                             InvalidationTracker& invalidation) :
        tree_(tree),
        mouse_(mouse),
        keyboard_(keyboard),
        context_(context),
        backend_(backend),
        scheduler_(scheduler),
        msg_pump_(msg_pump),
        invalidation_(invalidation) {}

    auto EventRouter::dispatch(const platform::Event& event) const -> void {
        std::visit(platform::Overloaded{
                       [&](const device::MouseMoveEvent& e) -> void {
                           mouse_.handle(e);
                           handle_input(event);
                       },
                       [&](const device::MouseButtonEvent& e) -> void {
                           mouse_.handle(e);
                           handle_input(event);
                       },
                       [&](const device::MouseWheelEvent& e) -> void {
                           mouse_.handle(e);
                           handle_input(event);
                       },
                       [&](const device::KeyEvent& e) -> void {
                           keyboard_.handle(e);
                           handle_input(event);
                       },
                       [&](const device::CharEvent& e) -> void {
                           keyboard_.handle(e);
                           handle_input(event);
                       },
                       [&](const platform::ResizeEvent& e) -> void {
                           handle_resize(e);
                       },
                       [&](const platform::DpiChangeEvent& e) -> void {
                           handle_dpi_change(e);
                       },
                       [&](const platform::DestroyEvent&) -> void {
                           handle_destroy();
                       },
                   },
                   event);

        if (invalidation_.is_dirty() && !scheduler_.expired()) {
            scheduler_.lock()->request_frame();
        }
    }

    auto EventRouter::handle_input(const platform::Event& event) const -> void {
        const auto root = tree_.get_root();
        if (!root) {
            return;
        }
    }

    auto EventRouter::handle_resize(const platform::ResizeEvent& e) const -> void {
        if (!scheduler_.expired()) {
            scheduler_.lock()->set_pending_size(e.width, e.height);
        }
        invalidation_.mark_dirty();
    }

    auto EventRouter::handle_dpi_change(const platform::DpiChangeEvent& e) const -> void {
        backend_.set_dpi(e.dpi);
        mouse_.set_dpi(e.dpi);
        invalidation_.mark_dirty();
    }

    auto EventRouter::handle_destroy() const -> void {
        if (!scheduler_.expired()) {
            scheduler_.lock()->stop();
        }
        if (!msg_pump_.expired()) {
            msg_pump_.lock()->request_stop();
        }
    }
}
