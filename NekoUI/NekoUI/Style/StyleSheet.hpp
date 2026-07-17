#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "CSS.hpp"

namespace neko::style {
    using namespace neko::type;

    struct WidgetStyle {
        std::optional<Background> background;
        std::optional<Size> size;
        std::optional<Border> border;
        std::optional<float> font_size;
        std::optional<Color> text_color;
        std::optional<float> padding;
        std::optional<float> spacing;
    };

    class StyleSheet {
    public:
        auto add(std::string name, const WidgetStyle& style) -> StyleSheet& {
            rules_[std::move(name)] = style;
            return *this;
        }

        [[nodiscard]] auto get(const std::string_view name) const -> const WidgetStyle* {
            if (const auto it = rules_.find(std::string(name)); it != rules_.end()) {
                return &it->second;
            }
            return nullptr;
        }
    private:
        std::unordered_map<std::string, WidgetStyle> rules_;
    };
} // namespace neko::style
