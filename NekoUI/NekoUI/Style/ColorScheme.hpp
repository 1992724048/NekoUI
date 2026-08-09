#pragma once

#include <cstdint>

#include "../Type.hpp"

namespace neko::style {
    struct ColorScheme {
        enum class Brightness : uint8_t { Light, Dark };

        Brightness brightness = Brightness::Light;

        type::Color primary{};
        type::Color on_primary{};
        type::Color primary_container{};
        type::Color on_primary_container{};
        type::Color primary_fixed{};
        type::Color primary_fixed_dim{};
        type::Color on_primary_fixed{};
        type::Color on_primary_fixed_variant{};

        type::Color secondary{};
        type::Color on_secondary{};
        type::Color secondary_container{};
        type::Color on_secondary_container{};
        type::Color secondary_fixed{};
        type::Color secondary_fixed_dim{};
        type::Color on_secondary_fixed{};
        type::Color on_secondary_fixed_variant{};

        type::Color tertiary{};
        type::Color on_tertiary{};
        type::Color tertiary_container{};
        type::Color on_tertiary_container{};
        type::Color tertiary_fixed{};
        type::Color tertiary_fixed_dim{};
        type::Color on_tertiary_fixed{};
        type::Color on_tertiary_fixed_variant{};

        type::Color error{};
        type::Color on_error{};
        type::Color error_container{};
        type::Color on_error_container{};

        type::Color surface{};
        type::Color surface_dim{};
        type::Color surface_bright{};
        type::Color surface_container_lowest{};
        type::Color surface_container_low{};
        type::Color surface_container{};
        type::Color surface_container_high{};
        type::Color surface_container_highest{};
        type::Color on_surface{};
        type::Color surface_variant{};
        type::Color on_surface_variant{};
        type::Color surface_tint{};

        type::Color outline{};
        type::Color outline_variant{};

        type::Color shadow{};
        type::Color scrim{};

        type::Color inverse_surface{};
        type::Color inverse_on_surface{};
        type::Color inverse_primary{};

        [[nodiscard]] static auto light(type::Color seed) -> ColorScheme;
        [[nodiscard]] static auto dark(type::Color seed) -> ColorScheme;
    };
} // namespace neko::style
