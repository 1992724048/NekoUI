#pragma once
#include <list>
#include <variant>
#include <vector>

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    class MutableWidget;

    using MutableWidgetList = std::list<MutableWidget>;
    using MutableWidgetVector = std::vector<MutableWidget>;
    using WidgetContainer = std::variant<MutableWidgetList, MutableWidgetVector, widget::Widget>;

    class MutableWidget : public WidgetContainer {
    public:
        using Super = WidgetContainer;
        using Super::Super;
        using Super::operator=;

        explicit MutableWidget() = default;

        [[nodiscard]] auto is_null() const -> bool {
            return std::holds_alternative<std::monostate>(*this);
        }

        [[nodiscard]] auto is_widget() const -> bool {
            return std::holds_alternative<widget::Widget>(*this);
        }

        [[nodiscard]] auto is_list() const -> bool {
            return std::holds_alternative<MutableWidgetList>(*this);
        }

        [[nodiscard]] auto is_vector() const -> bool {
            return std::holds_alternative<MutableWidgetVector>(*this);
        }

        template<typename T>
        [[nodiscard]] auto get() const -> const T& {
            return std::get<T>(*this);
        }

        template<typename T>
        auto get() -> T& {
            return std::get<T>(*this);
        }

        template<typename T>
        auto get_if() -> T* {
            return std::get_if<T>(this);
        }

        [[nodiscard]] auto as_widget() const -> const widget::Widget& {
            return get<widget::Widget>();
        }

        [[nodiscard]] auto as_widget() -> widget::Widget& {
            return get<widget::Widget>();
        }

        [[nodiscard]] auto as_list() const -> const MutableWidgetList& {
            return std::get<MutableWidgetList>(*this);
        }

        [[nodiscard]] auto as_list() -> MutableWidgetList& {
            return std::get<MutableWidgetList>(*this);
        }

        [[nodiscard]] auto as_vector() const -> const MutableWidgetVector& {
            return std::get<MutableWidgetVector>(*this);
        }

        [[nodiscard]] auto as_vector() -> MutableWidgetVector& {
            return std::get<MutableWidgetVector>(*this);
        }
    };
}
