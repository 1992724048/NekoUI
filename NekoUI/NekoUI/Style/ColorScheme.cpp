#include "ColorScheme.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace neko::style {
    namespace {
        // ── HSL conversion helpers ─────────────────────────────────────────
        struct Hsl {
            float h; // 0-360
            float s; // 0-1
            float l; // 0-1
        };

        auto to_hsl(const type::Color c) -> Hsl {
            float r = c.r() / 255.0F;
            float g = c.g() / 255.0F;
            float b = c.b() / 255.0F;

            const float mx = std::max({r, g, b});
            const float mn = std::min({r, g, b});
            const float delta = mx - mn;

            float h = 0.0F;
            float s = 0.0F;
            const float l = (mx + mn) * 0.5F;

            if (delta > 0.001F) {
                s = (l > 0.5F) ? delta / (2.0F - mx - mn) : delta / (mx + mn);
                if (mx == r) {
                    h = std::fmod((g - b) / delta, 6.0F);
                } else if (mx == g) {
                    h = (b - r) / delta + 2.0F;
                } else {
                    h = (r - g) / delta + 4.0F;
                }
                h *= 60.0F;
                if (h < 0.0F) {
                    h += 360.0F;
                }
            }
            return {h, s, l};
        }

        auto from_hsl(const Hsl hsl) -> type::Color {
            const float h = hsl.h;
            const float s = hsl.s;
            const float l = hsl.l;

            auto hue_to_rgb = [](const float p, const float q, float t) -> float {
                if (t < 0.0F) {
                    t += 1.0F;
                }
                if (t > 1.0F) {
                    t -= 1.0F;
                }
                if (t < 1.0F / 6.0F) {
                    return p + (q - p) * 6.0F * t;
                }
                if (t < 1.0F / 2.0F) {
                    return q;
                }
                if (t < 2.0F / 3.0F) {
                    return p + (q - p) * (2.0F / 3.0F - t) * 6.0F;
                }
                return p;
            };

            // Achromatic
            if (s < 0.001F) {
                const uint8_t v = static_cast<uint8_t>(std::clamp(l * 255.0F, 0.0F, 255.0F));
                return type::Color{.value = (static_cast<uint32_t>(v) << 24) | (static_cast<uint32_t>(v) << 16) | (static_cast<uint32_t>(v) << 8) | 0xFFU};
            }

            const float q = (l < 0.5F) ? l * (1.0F + s) : l + s - l * s;
            const float p = 2.0F * l - q;
            const float h_norm = h / 360.0F;

            const auto r = static_cast<uint8_t>(std::clamp(hue_to_rgb(p, q, h_norm + 1.0F / 3.0F) * 255.0F, 0.0F, 255.0F));
            const auto g = static_cast<uint8_t>(std::clamp(hue_to_rgb(p, q, h_norm) * 255.0F, 0.0F, 255.0F));
            const auto b = static_cast<uint8_t>(std::clamp(hue_to_rgb(p, q, h_norm - 1.0F / 3.0F) * 255.0F, 0.0F, 255.0F));

            return type::Color{.value = (static_cast<uint32_t>(r) << 24) | (static_cast<uint32_t>(g) << 16) | (static_cast<uint32_t>(b) << 8) | 0xFFU};
        }

        // ── Tonal palette ──────────────────────────────────────────────────

        /// 13-key tonal palette matching M3 key tones: 0, 10, 20, 30, 40, 50,
        /// 60, 70, 80, 90, 95, 99, 100.
        struct TonalPalette {
            type::Color t0; // black
            type::Color t10;
            type::Color t20;
            type::Color t30;
            type::Color t40; // base role colour
            type::Color t50;
            type::Color t60;
            type::Color t70;
            type::Color t80;
            type::Color t90; // container role colour
            type::Color t95;
            type::Color t99;
            type::Color t100; // white
        };

        /// Generate a tonal palette from a hue/saturation pair.
        ///
        /// Saturation follows a sine curve centred on tone 50, so the colour is
        /// most vivid in the mid range and naturally desaturates toward black
        /// (tone 0) and white (tone 100), mimicking the M3 tonal behaviour.
        auto make_palette(float hue, float saturation) -> TonalPalette {
            auto tone = [hue, saturation](const float l) -> type::Color {
                // bell curve: peak saturation at tone 50, 0 at extremes
                float s_curve = saturation * std::sin(l * std::numbers::pi_v<float>);
                s_curve = std::max(0.0F, std::min(1.0F, s_curve));
                return from_hsl({.h = hue, .s = s_curve, .l = l});
            };
            return {
                .t0 = tone(0.00F),
                .t10 = tone(0.10F),
                .t20 = tone(0.20F),
                .t30 = tone(0.30F),
                .t40 = tone(0.40F),
                .t50 = tone(0.50F),
                .t60 = tone(0.60F),
                .t70 = tone(0.70F),
                .t80 = tone(0.80F),
                .t90 = tone(0.90F),
                .t95 = tone(0.95F),
                .t99 = tone(0.99F),
                .t100 = tone(1.00F),
            };
        }

        // ── Material You hue offsets ───────────────────────────────────────

        /// M3 default hue shifts for a single seed colour (applied in HCL/CAM16
        /// in the reference implementation; approximated here in HSL).
        constexpr float kSecondaryHueOffset = 30.0F; // primary + 30°
        constexpr float kTertiaryHueOffset = 30.0F; // primary - 30°
        constexpr float kNeutralSaturation = 0.04F; // nearly achromatic
        constexpr float kNeutralVarSat = 0.10F; // slightly more chroma
        constexpr float kSecondarySatScale = 0.60F; // secondary less vivid
        constexpr float kTertiarySatScale = 0.80F;
        constexpr float kErrorHue = 0.0F; // red
        constexpr float kErrorSaturation = 0.84F;

        // ── Role mapping helpers ───────────────────────────────────────────

        /// Light-scheme tone index for each role (index into TonalPalette).
        /// Suffix _L = light scheme, _D = dark scheme.
        enum ToneIdx : size_t {
            T0   = 0,
            T10  = 1,
            T20  = 2,
            T30  = 3,
            T40  = 4,
            T50  = 5,
            T60  = 6,
            T70  = 7,
            T80  = 8,
            T90  = 9,
            T95  = 10,
            T99  = 11,
            T100 = 12,
        };

        // "Dark" surface uses tone 6 — approximate as lerp(t0, t10, 0.6).
        auto surface_dark(const TonalPalette& neutral) -> type::Color {
            // Interpolate between tone 0 and tone 10 to get tone ~6
            auto lerp = [](const uint8_t a, const uint8_t b, const float t) -> uint8_t {
                return static_cast<uint8_t>(a + (b - a) * t + 0.5F);
            };
            return type::Color{
                .value = (static_cast<uint32_t>(lerp(neutral.t0.r(), neutral.t10.r(), 0.6F)) << 24) | (static_cast<uint32_t>(lerp(neutral.t0.g(), neutral.t10.g(), 0.6F)) << 16) | (static_cast<uint32_t>(lerp(
                        neutral.t0.b(),
                        neutral.t10.b(),
                        0.6F)) << 8) | 0xFFU
            };
        }
    } // anonymous namespace

    // =========================================================================
    //  Public factories
    // =========================================================================

    auto ColorScheme::light(type::Color seed) -> ColorScheme {
        auto [h, s, l] = to_hsl(seed);

        TonalPalette primary = make_palette(h, s);
        TonalPalette secondary = make_palette(std::fmod(h + kSecondaryHueOffset, 360.0F), s * kSecondarySatScale);
        TonalPalette tertiary = make_palette(std::fmod(h - kTertiaryHueOffset + 360.0F, 360.0F), s * kTertiarySatScale);
        TonalPalette neutral = make_palette(h, kNeutralSaturation);
        TonalPalette neutralVariant = make_palette(h, kNeutralVarSat);
        TonalPalette error = make_palette(kErrorHue, kErrorSaturation);

        return ColorScheme{
            .brightness = Brightness::Light,

            // Primary
            .primary = primary.t40,
            .onPrimary = primary.t100,
            .primaryContainer = primary.t90,
            .onPrimaryContainer = primary.t10,

            // Secondary
            .secondary = secondary.t40,
            .onSecondary = secondary.t100,
            .secondaryContainer = secondary.t90,
            .onSecondaryContainer = secondary.t10,

            // Tertiary
            .tertiary = tertiary.t40,
            .onTertiary = tertiary.t100,
            .tertiaryContainer = tertiary.t90,
            .onTertiaryContainer = tertiary.t10,

            // Error
            .error = error.t40,
            .onError = error.t100,
            .errorContainer = error.t90,
            .onErrorContainer = error.t10,

            // Surface
            .surface = neutral.t99,
            .onSurface = neutral.t10,
            .surfaceVariant = neutralVariant.t90,
            .onSurfaceVariant = neutralVariant.t30,

            // Outline
            .outline = neutralVariant.t50,
            .outlineVariant = neutralVariant.t80,

            // Shadow / Scrim
            .shadow = neutral.t0,
            .scrim = neutral.t0,

            // Inverse
            .inverseSurface = neutral.t20,
            .inverseOnSurface = neutral.t95,
            .inversePrimary = primary.t80,
        };
    }

    auto ColorScheme::dark(type::Color seed) -> ColorScheme {
        auto [h, s, l] = to_hsl(seed);

        TonalPalette primary = make_palette(h, s);
        TonalPalette secondary = make_palette(std::fmod(h + kSecondaryHueOffset, 360.0F), s * kSecondarySatScale);
        TonalPalette tertiary = make_palette(std::fmod(h - kTertiaryHueOffset + 360.0F, 360.0F), s * kTertiarySatScale);
        TonalPalette neutral = make_palette(h, kNeutralSaturation);
        TonalPalette neutralVariant = make_palette(h, kNeutralVarSat);
        TonalPalette error = make_palette(kErrorHue, kErrorSaturation);

        return ColorScheme{
            .brightness = Brightness::Dark,

            // Primary
            .primary = primary.t80,
            .onPrimary = primary.t20,
            .primaryContainer = primary.t30,
            .onPrimaryContainer = primary.t90,

            // Secondary
            .secondary = secondary.t80,
            .onSecondary = secondary.t20,
            .secondaryContainer = secondary.t30,
            .onSecondaryContainer = secondary.t90,

            // Tertiary
            .tertiary = tertiary.t80,
            .onTertiary = tertiary.t20,
            .tertiaryContainer = tertiary.t30,
            .onTertiaryContainer = tertiary.t90,

            // Error
            .error = error.t80,
            .onError = error.t20,
            .errorContainer = error.t30,
            .onErrorContainer = error.t90,

            // Surface
            .surface = surface_dark(neutral),
            .onSurface = neutral.t90,
            .surfaceVariant = neutralVariant.t30,
            .onSurfaceVariant = neutralVariant.t80,

            // Outline
            .outline = neutralVariant.t60,
            .outlineVariant = neutralVariant.t30,

            // Shadow / Scrim
            .shadow = neutral.t0,
            .scrim = neutral.t0,

            // Inverse
            .inverseSurface = neutral.t90,
            .inverseOnSurface = neutral.t20,
            .inversePrimary = primary.t40,
        };
    }
} // namespace neko::style
