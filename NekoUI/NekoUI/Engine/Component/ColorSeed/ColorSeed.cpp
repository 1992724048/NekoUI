#include "ColorSeed.hpp"

namespace neko::color {
    auto ColorScheme::from_seed(const type::Color seed, const bool is_dark, const float contrast_level) -> ColorScheme {
        const auto argb = detail::argb_from_rgb(
            static_cast<int>(std::round(seed.r * 255.0F)),
            static_cast<int>(std::round(seed.g * 255.0F)),
            static_cast<int>(std::round(seed.b * 255.0F)));
        const auto hct = detail::Hct::from_int(argb);
        const detail::DynamicScheme scheme(hct, is_dark, contrast_level);

        const auto resolve = [&](const detail::DynamicColor& role) -> type::Color {
            return detail::argb_to_color(role.get_argb(scheme));
        };

        const auto resolved_primary = resolve(detail::MaterialDynamicColors::primary(scheme));

        return {
            .primary = resolved_primary,
            .on_primary = resolve(detail::MaterialDynamicColors::on_primary(scheme)),
            .primary_container = resolve(detail::MaterialDynamicColors::primary_container(scheme)),
            .on_primary_container = resolve(detail::MaterialDynamicColors::on_primary_container(scheme)),
            .primary_fixed = resolve(detail::MaterialDynamicColors::primary_fixed(scheme)),
            .primary_fixed_dim = resolve(detail::MaterialDynamicColors::primary_fixed_dim(scheme)),
            .on_primary_fixed = resolve(detail::MaterialDynamicColors::on_primary_fixed(scheme)),
            .on_primary_fixed_variant = resolve(detail::MaterialDynamicColors::on_primary_fixed_variant(scheme)),

            .secondary = resolve(detail::MaterialDynamicColors::secondary(scheme)),
            .on_secondary = resolve(detail::MaterialDynamicColors::on_secondary(scheme)),
            .secondary_container = resolve(detail::MaterialDynamicColors::secondary_container(scheme)),
            .on_secondary_container = resolve(detail::MaterialDynamicColors::on_secondary_container(scheme)),
            .secondary_fixed = resolve(detail::MaterialDynamicColors::secondary_fixed(scheme)),
            .secondary_fixed_dim = resolve(detail::MaterialDynamicColors::secondary_fixed_dim(scheme)),
            .on_secondary_fixed = resolve(detail::MaterialDynamicColors::on_secondary_fixed(scheme)),
            .on_secondary_fixed_variant = resolve(detail::MaterialDynamicColors::on_secondary_fixed_variant(scheme)),

            .tertiary = resolve(detail::MaterialDynamicColors::tertiary(scheme)),
            .on_tertiary = resolve(detail::MaterialDynamicColors::on_tertiary(scheme)),
            .tertiary_container = resolve(detail::MaterialDynamicColors::tertiary_container(scheme)),
            .on_tertiary_container = resolve(detail::MaterialDynamicColors::on_tertiary_container(scheme)),
            .tertiary_fixed = resolve(detail::MaterialDynamicColors::tertiary_fixed(scheme)),
            .tertiary_fixed_dim = resolve(detail::MaterialDynamicColors::tertiary_fixed_dim(scheme)),
            .on_tertiary_fixed = resolve(detail::MaterialDynamicColors::on_tertiary_fixed(scheme)),
            .on_tertiary_fixed_variant = resolve(detail::MaterialDynamicColors::on_tertiary_fixed_variant(scheme)),

            .error = resolve(detail::MaterialDynamicColors::error(scheme)),
            .on_error = resolve(detail::MaterialDynamicColors::on_error(scheme)),
            .error_container = resolve(detail::MaterialDynamicColors::error_container(scheme)),
            .on_error_container = resolve(detail::MaterialDynamicColors::on_error_container(scheme)),

            .surface = resolve(detail::MaterialDynamicColors::surface(scheme)),
            .surface_dim = resolve(detail::MaterialDynamicColors::surface_dim(scheme)),
            .surface_bright = resolve(detail::MaterialDynamicColors::surface_bright(scheme)),
            .surface_container_lowest = resolve(detail::MaterialDynamicColors::surface_container_lowest(scheme)),
            .surface_container_low = resolve(detail::MaterialDynamicColors::surface_container_low(scheme)),
            .surface_container = resolve(detail::MaterialDynamicColors::surface_container(scheme)),
            .surface_container_high = resolve(detail::MaterialDynamicColors::surface_container_high(scheme)),
            .surface_container_highest = resolve(detail::MaterialDynamicColors::surface_container_highest(scheme)),
            .on_surface = resolve(detail::MaterialDynamicColors::on_surface(scheme)),
            .on_surface_variant = resolve(detail::MaterialDynamicColors::on_surface_variant(scheme)),

            .outline = resolve(detail::MaterialDynamicColors::outline(scheme)),
            .outline_variant = resolve(detail::MaterialDynamicColors::outline_variant(scheme)),

            .inverse_surface = resolve(detail::MaterialDynamicColors::inverse_surface(scheme)),
            .inverse_on_surface = resolve(detail::MaterialDynamicColors::inverse_on_surface(scheme)),
            .inverse_primary = resolve(detail::MaterialDynamicColors::inverse_primary(scheme)),

            .shadow = resolve(detail::MaterialDynamicColors::shadow(scheme)),
            .scrim = resolve(detail::MaterialDynamicColors::scrim(scheme)),
            .surface_tint = resolved_primary,

            .is_dark = is_dark,
        };
    }
} // namespace neko::color
