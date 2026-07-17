#pragma once
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "CSS.hpp"

namespace neko::style {
    using namespace neko::type;

    /// 统一的 Widget 样式属性（所有 Widget 类型共用）
    struct WidgetStyle {
        std::optional<Background> background;
        std::optional<Size> size;
        std::optional<Border> border;
        std::optional<float> font_size;
        std::optional<Color> text_color;
        std::optional<float> padding;
        std::optional<float> spacing;
    };

    /// 全局样式表，按 class_name 索引
    class StyleSheet {
    public:
        auto add(std::string name, WidgetStyle style) -> StyleSheet& {
            rules_[std::move(name)] = std::move(style);
            return *this;
        }

        [[nodiscard]] auto get(std::string_view name) const -> const WidgetStyle* {
            if (auto it = rules_.find(std::string(name)); it != rules_.end()) {
                return &it->second;
            }
            return nullptr;
        }

    private:
        std::unordered_map<std::string, WidgetStyle> rules_;
    };
} // namespace neko::style
