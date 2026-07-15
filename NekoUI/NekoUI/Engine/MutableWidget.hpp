#pragma once
#include <list>
#include <memory>
#include <variant>
#include <vector>

namespace neko::widget {
    class Widget;
}

namespace neko::engine {
    class MutableWidget;

    namespace internal {
        using MutableWidgetList = std::list<MutableWidget>;
        using MutableWidgetVector = std::vector<MutableWidget>;
        using WidgetContainer = std::variant<std::monostate, MutableWidgetList, MutableWidgetVector, std::weak_ptr<widget::Widget>>;
    }

    class MutableWidget : public internal::WidgetContainer {
    public:
        using Super = internal::WidgetContainer;
        using Super::Super;
        using Super::operator=;

        explicit MutableWidget() = default;

        [[nodiscard]] auto is_null() const -> bool {
            return std::holds_alternative<std::monostate>(*this);
        }

        [[nodiscard]] auto is_widget() const -> bool {
            return std::holds_alternative<std::weak_ptr<widget::Widget>>(*this);
        }

        [[nodiscard]] auto is_list() const -> bool {
            return std::holds_alternative<internal::MutableWidgetList>(*this);
        }

        [[nodiscard]] auto is_vector() const -> bool {
            return std::holds_alternative<internal::MutableWidgetVector>(*this);
        }

        template<typename T>
        [[nodiscard]] auto get() const -> const T& {
            return std::get<T>(static_cast<const Super&>(*this));
        }

        template<typename T>
        auto get() -> T& {
            return std::get<T>(static_cast<Super&>(*this));
        }

        template<typename T>
        auto get_if() -> T* {
            return std::get_if<T>(static_cast<Super*>(this));
        }

        [[nodiscard]] auto as_widget() const -> const std::weak_ptr<widget::Widget>& {
            return get<std::weak_ptr<widget::Widget>>();
        }

        [[nodiscard]] auto as_widget() -> std::weak_ptr<widget::Widget>& {
            return get<std::weak_ptr<widget::Widget>>();
        }

        [[nodiscard]] auto as_list() const -> const internal::MutableWidgetList& {
            return std::get<internal::MutableWidgetList>(*this);
        }

        [[nodiscard]] auto as_list() -> internal::MutableWidgetList& {
            return std::get<internal::MutableWidgetList>(*this);
        }

        [[nodiscard]] auto as_vector() const -> const internal::MutableWidgetVector& {
            return std::get<internal::MutableWidgetVector>(*this);
        }

        [[nodiscard]] auto as_vector() -> internal::MutableWidgetVector& {
            return std::get<internal::MutableWidgetVector>(*this);
        }
    };
}
