#pragma once
#include <cstdint>
#include "../Type.hpp"

namespace neko::style {
    /// Material You (Material Design 3) tonal ColorScheme.
    ///
    /// Each scheme is built from a seed color, which drives five tonal palettes:
    /// primary, secondary, tertiary, neutral, and neutralVariant.
    /// These palettes supply the colour roles per the M3 specification.
    ///
    /// Usage:
    ///   auto light = ColorScheme::light(seed);
    ///   auto dark  = ColorScheme::dark(seed);
    struct ColorScheme {
        enum class Brightness : uint8_t { Light, Dark };

        Brightness brightness = Brightness::Light;

        // ── Primary ──────────────────────────────────────────
        type::Color primary{}; // Key brand colour — tone 40 (light) / 80 (dark)
        type::Color onPrimary{}; // Content on primary
        type::Color primaryContainer{}; // Softer primary background — tone 90 / 30
        type::Color onPrimaryContainer{}; // Content on primary container

        // ── Secondary ────────────────────────────────────────
        type::Color secondary{}; // Less prominent accent — tone 40 / 80
        type::Color onSecondary{};
        type::Color secondaryContainer{}; // tone 90 / 30
        type::Color onSecondaryContainer{};

        // ── Tertiary ─────────────────────────────────────────
        type::Color tertiary{}; // Complementary accent — tone 40 / 80
        type::Color onTertiary{};
        type::Color tertiaryContainer{}; // tone 90 / 30
        type::Color onTertiaryContainer{};

        // ── Error ────────────────────────────────────────────
        type::Color error{}; // Fixed red-tone 40 / 80
        type::Color onError{};
        type::Color errorContainer{}; // tone 90 / 30
        type::Color onErrorContainer{};

        // ── Surface / Background ─────────────────────────────
        type::Color surface{}; // App background — neutral tone 98 / 6
        type::Color onSurface{}; // Primary text on surface — tone 10 / 90
        type::Color surfaceVariant{}; // Slightly deeper background — neutralVariant tone 90 / 30
        type::Color onSurfaceVariant{}; // Secondary text — neutralVariant tone 30 / 80

        // ── Outline ──────────────────────────────────────────
        type::Color outline{}; // Borders, dividers — neutralVariant tone 50 / 60
        type::Color outlineVariant{}; // Softer borders — neutralVariant tone 80 / 30

        // ── Shadow / Scrim ───────────────────────────────────
        type::Color shadow{}; // Drop shadow (tone 0)
        type::Color scrim{}; // Scrim overlay (tone 0)

        // ── Inverse ──────────────────────────────────────────
        type::Color inverseSurface{}; // Inverse app bg — neutral tone 20 / 90
        type::Color inverseOnSurface{}; // Content on inverse surface — tone 95 / 20
        type::Color inversePrimary{}; // Primary on dark bg — tone 80 / 40

        static auto light(type::Color seed) -> ColorScheme;
        static auto dark(type::Color seed) -> ColorScheme;
    };
} // namespace neko::style
