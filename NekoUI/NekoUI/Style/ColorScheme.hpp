#pragma once

#include <cstdint>

#include "../Type.hpp"

namespace neko::style {
    struct ColorScheme {
        enum class Brightness : uint8_t { Light, Dark };

        Brightness brightness = Brightness::Light;

        type::Color primary{};
        type::Color onPrimary{};
        type::Color primaryContainer{};
        type::Color onPrimaryContainer{};
        type::Color primaryFixed{};
        type::Color primaryFixedDim{};
        type::Color onPrimaryFixed{};
        type::Color onPrimaryFixedVariant{};

        type::Color secondary{};
        type::Color onSecondary{};
        type::Color secondaryContainer{};
        type::Color onSecondaryContainer{};
        type::Color secondaryFixed{};
        type::Color secondaryFixedDim{};
        type::Color onSecondaryFixed{};
        type::Color onSecondaryFixedVariant{};

        type::Color tertiary{};
        type::Color onTertiary{};
        type::Color tertiaryContainer{};
        type::Color onTertiaryContainer{};
        type::Color tertiaryFixed{};
        type::Color tertiaryFixedDim{};
        type::Color onTertiaryFixed{};
        type::Color onTertiaryFixedVariant{};

        type::Color error{};
        type::Color onError{};
        type::Color errorContainer{};
        type::Color onErrorContainer{};

        type::Color surface{};
        type::Color surfaceDim{};
        type::Color surfaceBright{};
        type::Color surfaceContainerLowest{};
        type::Color surfaceContainerLow{};
        type::Color surfaceContainer{};
        type::Color surfaceContainerHigh{};
        type::Color surfaceContainerHighest{};
        type::Color onSurface{};
        type::Color surfaceVariant{};
        type::Color onSurfaceVariant{};
        type::Color surfaceTint{};

        type::Color outline{};
        type::Color outlineVariant{};

        type::Color shadow{};
        type::Color scrim{};

        type::Color inverseSurface{};
        type::Color inverseOnSurface{};
        type::Color inversePrimary{};

        [[nodiscard]] static auto light(type::Color seed) -> ColorScheme;
        [[nodiscard]] static auto dark(type::Color seed) -> ColorScheme;
    };
} // namespace neko::style
