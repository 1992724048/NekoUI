#pragma once
#include "../Widget/Widget.hpp"

namespace neko::engine {
    template<typename Visitor>
    auto visit_children(widget::Widget& w, Visitor&& visit) -> void {
        auto& children = w.get_children();
        if (children.is_null()) {
            return;
        }

        if (children.is_widget()) {
            visit(children.as_widget());
        } else if (children.is_list()) {
            for (auto& mw : children.as_list()) {
                if (mw.is_widget()) {
                    visit(mw.as_widget());
                }
            }
        } else if (children.is_vector()) {
            for (auto& mw : children.as_vector()) {
                if (mw.is_widget()) {
                    visit(mw.as_widget());
                }
            }
        }
    }
} // namespace neko::engine
