// 2026-08-10

#pragma once

namespace neko::widget {
    class Widget;
}

namespace neko::behavior {
    // 行为基类：持 Widget& 访问控件数据（bounds/style/交互状态），禁拷贝（引用成员）
    class Behavior {
    public:
        explicit Behavior(neko::widget::Widget& owner) : owner_{owner} {}
        virtual ~Behavior() = default;

        Behavior(const Behavior&) = delete;
        auto operator=(const Behavior&) -> Behavior& = delete;
        Behavior(Behavior&&) = delete;
        auto operator=(Behavior&&) -> Behavior& = delete;
    protected:
        neko::widget::Widget& owner_;
    };
}
