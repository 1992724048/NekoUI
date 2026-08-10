// 2026-08-10 10:18:21

#pragma once

namespace neko::widget {
    class Widget;
}

namespace neko::behavior {
    class Behavior {
    public:
        explicit Behavior(widget::Widget& owner) :
            owner_{owner} {}

        virtual ~Behavior() = default;

        Behavior(const Behavior&) = delete;
        auto operator=(const Behavior&) -> Behavior& = delete;
        Behavior(Behavior&&) = delete;
        auto operator=(Behavior&&) -> Behavior& = delete;
    protected:
        widget::Widget& owner_;
    };
}
