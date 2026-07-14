#pragma once
#include "../../../Type.hpp"

namespace neko::color {
    // Generated from a single seed color via HCT tonal palette -> from_seed().
    struct ColorScheme {
        // -- Primary group --
        type::Color primary;
        type::Color on_primary;
        type::Color primary_container;
        type::Color on_primary_container;
        type::Color primary_fixed;
        type::Color primary_fixed_dim;
        type::Color on_primary_fixed;
        type::Color on_primary_fixed_variant;

        // -- Secondary group --
        type::Color secondary;
        type::Color on_secondary;
        type::Color secondary_container;
        type::Color on_secondary_container;
        type::Color secondary_fixed;
        type::Color secondary_fixed_dim;
        type::Color on_secondary_fixed;
        type::Color on_secondary_fixed_variant;

        // -- Tertiary group --
        type::Color tertiary;
        type::Color on_tertiary;
        type::Color tertiary_container;
        type::Color on_tertiary_container;
        type::Color tertiary_fixed;
        type::Color tertiary_fixed_dim;
        type::Color on_tertiary_fixed;
        type::Color on_tertiary_fixed_variant;

        // -- Error group --
        type::Color error;
        type::Color on_error;
        type::Color error_container;
        type::Color on_error_container;

        // -- Surface group --
        type::Color surface;
        type::Color surface_dim;
        type::Color surface_bright;
        type::Color surface_container_lowest;
        type::Color surface_container_low;
        type::Color surface_container;
        type::Color surface_container_high;
        type::Color surface_container_highest;
        type::Color on_surface;
        type::Color on_surface_variant;

        // -- Outline group --
        type::Color outline;
        type::Color outline_variant;

        // -- Inverse group --
        type::Color inverse_surface;
        type::Color inverse_on_surface;
        type::Color inverse_primary;

        // -- Misc --
        type::Color shadow;
        type::Color scrim;
        type::Color surface_tint; // elevation overlay, typically equals primary

        bool is_dark{}; // theme brightness flag

        // Generate a full ColorScheme from a single seed color,
        // using HCT tonal palettes from material_color_utilities.
        static auto from_seed(type::Color seed, bool is_dark = false, float contrast_level = 0.0F) -> ColorScheme;
    };
} // namespace neko::color

