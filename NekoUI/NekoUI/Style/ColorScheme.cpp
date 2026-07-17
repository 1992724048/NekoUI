#include "ColorScheme.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace neko::style {
    namespace {
        struct Vec3 {
            float x, y, z;
        };

        [[nodiscard]] auto linearized(const uint8_t channel) -> float {
            const float v = channel / 255.0F;
            return v <= 0.04045F ? v / 12.92F : std::pow((v + 0.055F) / 1.055F, 2.4F);
        }

        [[nodiscard]] auto delinearized(const float channel) -> uint8_t {
            const float c = std::clamp(channel, 0.0F, 1.0F);
            const float v = c <= 0.0031308F ? 12.92F * c : 1.055F * std::pow(c, 1.0F / 2.4F) - 0.055F;
            return static_cast<uint8_t>(std::lround(std::clamp(v * 255.0F, 0.0F, 255.0F)));
        }

        [[nodiscard]] auto xyz_from_rgb(const uint8_t r, const uint8_t g, const uint8_t b) -> Vec3 {
            const float rl = linearized(r) * 100.0F;
            const float gl = linearized(g) * 100.0F;
            const float bl = linearized(b) * 100.0F;
            return {.x = 0.41233895F * rl + 0.35762064F * gl + 0.18051042F * bl, .y = 0.2126F * rl + 0.7152F * gl + 0.0722F * bl, .z = 0.01932141F * rl + 0.11916382F * gl + 0.95034478F * bl,};
        }

        constexpr float kEpsilon = 216.0F / 24389.0F;
        constexpr float kKappa = 24389.0F / 27.0F;

        [[nodiscard]] auto lstar_from_y(const float y) -> float {
            const float v = y / 100.0F;
            return v <= kEpsilon ? kKappa * v : 116.0F * std::cbrt(v) - 16.0F;
        }

        [[nodiscard]] auto y_from_lstar(const float lstar) -> float {
            return lstar <= 8.0F ? 100.0F * lstar / kKappa : 100.0F * std::pow((lstar + 16.0F) / 116.0F, 3.0F);
        }

        struct Cam16 {
            float hue;
            float chroma;
            float j;
        };

        struct ViewingConditions {
            std::array<float, 3> rgb_d;
            float fl;
            float n;
            float z;
            float nbb;
            float ncb;
            float aw;
            float c;
            float nc;
        };

        [[nodiscard]] auto chromatic_adaptation(const float component, const float fl) -> float {
            const float p = std::pow(fl * std::fabs(component) / 100.0F, 0.42F);
            return std::copysign(400.0F * p / (p + 27.13F), component);
        }

        [[nodiscard]] auto make_viewing_conditions() -> ViewingConditions {
            constexpr Vec3 kWhiteD65{.x = 95.047F, .y = 100.0F, .z = 108.883F};

            const float la = 200.0F / std::numbers::pi_v<float> * y_from_lstar(50.0F) / 100.0F;
            const float bg_y = y_from_lstar(50.0F);

            constexpr float rw = 0.401288F * kWhiteD65.x + 0.650173F * kWhiteD65.y - 0.051461F * kWhiteD65.z;
            constexpr float gw = -0.250268F * kWhiteD65.x + 1.204414F * kWhiteD65.y + 0.045854F * kWhiteD65.z;
            constexpr float bw = -0.002079F * kWhiteD65.x + 0.048952F * kWhiteD65.y + 0.953127F * kWhiteD65.z;

            const float d = std::clamp(1.0F - (1.0F / 3.6F) * std::exp((-la - 42.0F) / 92.0F), 0.0F, 1.0F);

            const float k = 1.0F / (5.0F * la + 1.0F);
            const float k4 = k * k * k * k;
            const float fl = 0.2F * k4 * (5.0F * la) + 0.1F * (1.0F - k4) * (1.0F - k4) * std::cbrt(5.0F * la);

            const float n = bg_y / kWhiteD65.y;
            const float nbb = 0.725F / std::pow(n, 0.2F);

            ViewingConditions vc{};
            vc.rgb_d[0] = d * (kWhiteD65.y / rw) + 1.0F - d;
            vc.rgb_d[1] = d * (kWhiteD65.y / gw) + 1.0F - d;
            vc.rgb_d[2] = d * (kWhiteD65.y / bw) + 1.0F - d;
            vc.fl = fl;
            vc.n = n;
            vc.z = 1.48F + std::sqrt(n);
            vc.nbb = nbb;
            vc.ncb = nbb;
            vc.aw = (2.0F * chromatic_adaptation(vc.rgb_d[0] * rw, fl) + chromatic_adaptation(vc.rgb_d[1] * gw, fl) + 0.05F * chromatic_adaptation(vc.rgb_d[2] * bw, fl) - 0.305F) * nbb;
            vc.c = 0.69F;
            vc.nc = 1.0F;
            return vc;
        }

        [[nodiscard]] auto viewing_conditions() -> const ViewingConditions& {
            static const ViewingConditions kInstance = make_viewing_conditions();
            return kInstance;
        }

        [[nodiscard]] auto cam16_from_xyz(const Vec3 xyz) -> Cam16 {
            const ViewingConditions& vc = viewing_conditions();

            const float r = 0.401288F * xyz.x + 0.650173F * xyz.y - 0.051461F * xyz.z;
            const float g = -0.250268F * xyz.x + 1.204414F * xyz.y + 0.045854F * xyz.z;
            const float b = -0.002079F * xyz.x + 0.048952F * xyz.y + 0.953127F * xyz.z;

            const float ra = chromatic_adaptation(vc.rgb_d[0] * r, vc.fl);
            const float ga = chromatic_adaptation(vc.rgb_d[1] * g, vc.fl);
            const float ba = chromatic_adaptation(vc.rgb_d[2] * b, vc.fl);

            const float a = ra + (-12.0F * ga + ba) / 11.0F;
            const float bb = (ra + ga - 2.0F * ba) / 9.0F;

            float hue = std::atan2(bb, a) * 180.0F / std::numbers::pi_v<float>;
            if (hue < 0.0F) {
                hue += 360.0F;
            }

            const float aw = (2.0F * ra + ga + 0.05F * ba - 0.305F) * vc.nbb;
            const float j = 100.0F * std::pow(aw / vc.aw, vc.c * vc.z);

            const float hue_rad = std::atan2(bb, a);
            const float e_t = 0.25F * (std::cos(hue_rad + 2.0F) + 3.8F);
            const float t = (50000.0F / 13.0F * vc.nc * vc.ncb * e_t * std::hypot(a, bb)) / (ra + ga + 1.05F * ba);
            const float alpha = std::pow(t, 0.9F) * std::pow(1.64F - std::pow(0.29F, vc.n), 0.73F);

            return {.hue = hue, .chroma = alpha * std::sqrt(j / 100.0F), .j = j};
        }

        struct Hct {
            float hue;
            float chroma;
            float tone;
        };

        [[nodiscard]] auto hct_from_color(const type::Color color) -> Hct {
            const Vec3 xyz = xyz_from_rgb(color.r(), color.g(), color.b());
            const Cam16 cam = cam16_from_xyz(xyz);
            return {.hue = cam.hue, .chroma = cam.chroma, .tone = lstar_from_y(xyz.y)};
        }

        [[nodiscard]] constexpr auto make_color(const uint8_t r, const uint8_t g, const uint8_t b) -> type::Color {
            return type::Color{.value = (static_cast<uint32_t>(r) << 24U) | (static_cast<uint32_t>(g) << 16U) | (static_cast<uint32_t>(b) << 8U) | 0xFFU,};
        }

        [[nodiscard]] auto inverse_chromatic_adaptation(const float adapted) -> float {
            const float abs_a = std::fabs(adapted);
            const float base = std::max(0.0F, 27.13F * abs_a / (400.0F - abs_a));
            return std::copysign(100.0F / viewing_conditions().fl * std::pow(base, 1.0F / 0.42F), adapted);
        }

        [[nodiscard]] auto find_linear_rgb(const float hue_deg, const float chroma, const float y, std::array<float, 3>& out) -> bool {
            const ViewingConditions& vc = viewing_conditions();

            if (chroma < 1e-4F) {
                out[0] = out[1] = out[2] = y;
                return true;
            }

            const float hue = hue_deg * std::numbers::pi_v<float> / 180.0F;
            const float t_inner = 1.0F / std::pow(1.64F - std::pow(0.29F, vc.n), 0.73F);
            const float e_hue = 0.25F * (std::cos(hue + 2.0F) + 3.8F);
            const float p1 = e_hue * (50000.0F / 13.0F) * vc.nc * vc.ncb;
            const float h_sin = std::sin(hue);
            const float h_cos = std::cos(hue);

            float j = std::sqrt(y) * 11.0F;
            for (int round = 0; round < 5; ++round) {
                const float jn = j / 100.0F;
                const float alpha = j <= 0.0F ? 0.0F : chroma / std::sqrt(jn);
                const float t = std::pow(alpha * t_inner, 1.0F / 0.9F);
                const float ac = vc.aw * std::pow(jn, 1.0F / vc.c / vc.z);
                const float p2 = ac / vc.nbb;
                const float gamma = 23.0F * (p2 + 0.305F) * t / (23.0F * p1 + 11.0F * t * h_cos + 108.0F * t * h_sin);
                const float a = gamma * h_cos;
                const float b = gamma * h_sin;
                const float ra = (460.0F * p2 + 451.0F * a + 288.0F * b) / 1403.0F;
                const float ga = (460.0F * p2 - 891.0F * a - 261.0F * b) / 1403.0F;
                const float ba = (460.0F * p2 - 220.0F * a - 6300.0F * b) / 1403.0F;

                const float rc = inverse_chromatic_adaptation(ra) / vc.rgb_d[0];
                const float gc = inverse_chromatic_adaptation(ga) / vc.rgb_d[1];
                const float bc = inverse_chromatic_adaptation(ba) / vc.rgb_d[2];

                const float x = 1.86206786F * rc - 1.01125463F * gc + 0.14918677F * bc;
                const float yy = 0.38752654F * rc + 0.62144744F * gc - 0.00897398F * bc;
                const float z = -0.01584150F * rc - 0.03412294F * gc + 1.04996444F * bc;

                const float lr = 3.2413775F * x - 1.5376652F * yy - 0.4988538F * z;
                const float lg = -0.9691424F * x + 1.8760108F * yy + 0.0415560F * z;
                const float lb = 0.0556209F * x - 0.2039559F * yy + 1.0572252F * z;

                if (lr < 0.0F || lg < 0.0F || lb < 0.0F) {
                    return false;
                }
                const float fnj = 0.2126F * lr + 0.7152F * lg + 0.0722F * lb;
                if (fnj <= 0.0F) {
                    return false;
                }
                if (round == 4 || std::fabs(fnj - y) < 0.002F) {
                    if (lr > 100.01F || lg > 100.01F || lb > 100.01F) {
                        return false;
                    }
                    out[0] = lr;
                    out[1] = lg;
                    out[2] = lb;
                    return true;
                }
                j -= (fnj - y) * j / (2.0F * fnj);
                j = std::max(j, 0.0F);
            }
            return false;
        }

        [[nodiscard]] auto hct_to_color(const Hct hct) -> type::Color {
            const float tone = std::clamp(hct.tone, 0.0F, 100.0F);
            if (tone <= 1e-4F) {
                return make_color(0, 0, 0);
            }
            if (tone >= 99.9999F) {
                return make_color(255, 255, 255);
            }

            const float y = y_from_lstar(tone);
            const float hue = std::fmod(std::fmod(hct.hue, 360.0F) + 360.0F, 360.0F);

            auto lin = std::array<float, 3>{y, y, y};
            if (!find_linear_rgb(hue, hct.chroma, y, lin)) {
                float lo = 0.0F;
                float hi = hct.chroma;
                for (int i = 0; i < 16; ++i) {
                    const float mid = 0.5F * (lo + hi);
                    auto candidate = std::array<float, 3>{};
                    if (find_linear_rgb(hue, mid, y, candidate)) {
                        lo = mid;
                        lin[0] = candidate[0];
                        lin[1] = candidate[1];
                        lin[2] = candidate[2];
                    } else {
                        hi = mid;
                    }
                }
                if (lo <= 1e-4F) {
                    lin[0] = lin[1] = lin[2] = y;
                }
            }

            return make_color(delinearized(lin[0] / 100.0F), delinearized(lin[1] / 100.0F), delinearized(lin[2] / 100.0F));
        }

        constexpr float kPrimaryChroma = 36.0F;
        constexpr float kSecondaryChroma = 16.0F;
        constexpr float kTertiaryChroma = 24.0F;
        constexpr float kTertiaryHueShift = 60.0F;
        constexpr float kNeutralChroma = 6.0F;
        constexpr float kNeutralVariantChroma = 8.0F;
        constexpr float kErrorHue = 25.0F;
        constexpr float kErrorChroma = 84.0F;

        struct TonalPalette {
            float hue;
            float chroma;

            [[nodiscard]] auto tone(const float t) const -> type::Color {
                return hct_to_color({.hue = hue, .chroma = chroma, .tone = t});
            }
        };

        struct Palettes {
            TonalPalette primary;
            TonalPalette secondary;
            TonalPalette tertiary;
            TonalPalette neutral;
            TonalPalette neutral_variant;
            TonalPalette error;
        };

        [[nodiscard]] auto make_palettes(const type::Color seed) -> Palettes {
            const Hct hct = hct_from_color(seed);
            return {
                .primary = {.hue = hct.hue, .chroma = kPrimaryChroma},
                .secondary = {.hue = hct.hue, .chroma = kSecondaryChroma},
                .tertiary = {.hue = std::fmod(hct.hue + kTertiaryHueShift, 360.0F), .chroma = kTertiaryChroma},
                .neutral = {.hue = hct.hue, .chroma = kNeutralChroma},
                .neutral_variant = {.hue = hct.hue, .chroma = kNeutralVariantChroma},
                .error = {.hue = kErrorHue, .chroma = kErrorChroma},
            };
        }
    } // namespace

    auto ColorScheme::light(const type::Color seed) -> ColorScheme {
        const auto [primary, secondary, tertiary, neutral, neutral_variant, error] = make_palettes(seed);
        return {
            .brightness = Brightness::Light,
            .primary = primary.tone(40),
            .onPrimary = primary.tone(100),
            .primaryContainer = primary.tone(90),
            .onPrimaryContainer = primary.tone(10),
            .primaryFixed = primary.tone(90),
            .primaryFixedDim = primary.tone(80),
            .onPrimaryFixed = primary.tone(10),
            .onPrimaryFixedVariant = primary.tone(30),
            .secondary = secondary.tone(40),
            .onSecondary = secondary.tone(100),
            .secondaryContainer = secondary.tone(90),
            .onSecondaryContainer = secondary.tone(10),
            .secondaryFixed = secondary.tone(90),
            .secondaryFixedDim = secondary.tone(80),
            .onSecondaryFixed = secondary.tone(10),
            .onSecondaryFixedVariant = secondary.tone(30),
            .tertiary = tertiary.tone(40),
            .onTertiary = tertiary.tone(100),
            .tertiaryContainer = tertiary.tone(90),
            .onTertiaryContainer = tertiary.tone(10),
            .tertiaryFixed = tertiary.tone(90),
            .tertiaryFixedDim = tertiary.tone(80),
            .onTertiaryFixed = tertiary.tone(10),
            .onTertiaryFixedVariant = tertiary.tone(30),
            .error = error.tone(40),
            .onError = error.tone(100),
            .errorContainer = error.tone(90),
            .onErrorContainer = error.tone(10),
            .surface = neutral.tone(98),
            .surfaceDim = neutral.tone(87),
            .surfaceBright = neutral.tone(98),
            .surfaceContainerLowest = neutral.tone(100),
            .surfaceContainerLow = neutral.tone(96),
            .surfaceContainer = neutral.tone(94),
            .surfaceContainerHigh = neutral.tone(92),
            .surfaceContainerHighest = neutral.tone(90),
            .onSurface = neutral.tone(10),
            .surfaceVariant = neutral_variant.tone(90),
            .onSurfaceVariant = neutral_variant.tone(30),
            .surfaceTint = primary.tone(40),
            .outline = neutral_variant.tone(50),
            .outlineVariant = neutral_variant.tone(80),
            .shadow = neutral.tone(0),
            .scrim = neutral.tone(0),
            .inverseSurface = neutral.tone(20),
            .inverseOnSurface = neutral.tone(95),
            .inversePrimary = primary.tone(80),
        };
    }

    auto ColorScheme::dark(const type::Color seed) -> ColorScheme {
        const auto [primary, secondary, tertiary, neutral, neutral_variant, error] = make_palettes(seed);
        return {
            .brightness = Brightness::Dark,
            .primary = primary.tone(80),
            .onPrimary = primary.tone(20),
            .primaryContainer = primary.tone(30),
            .onPrimaryContainer = primary.tone(90),
            .primaryFixed = primary.tone(90),
            .primaryFixedDim = primary.tone(80),
            .onPrimaryFixed = primary.tone(10),
            .onPrimaryFixedVariant = primary.tone(30),
            .secondary = secondary.tone(80),
            .onSecondary = secondary.tone(20),
            .secondaryContainer = secondary.tone(30),
            .onSecondaryContainer = secondary.tone(90),
            .secondaryFixed = secondary.tone(90),
            .secondaryFixedDim = secondary.tone(80),
            .onSecondaryFixed = secondary.tone(10),
            .onSecondaryFixedVariant = secondary.tone(30),
            .tertiary = tertiary.tone(80),
            .onTertiary = tertiary.tone(20),
            .tertiaryContainer = tertiary.tone(30),
            .onTertiaryContainer = tertiary.tone(90),
            .tertiaryFixed = tertiary.tone(90),
            .tertiaryFixedDim = tertiary.tone(80),
            .onTertiaryFixed = tertiary.tone(10),
            .onTertiaryFixedVariant = tertiary.tone(30),
            .error = error.tone(80),
            .onError = error.tone(20),
            .errorContainer = error.tone(30),
            .onErrorContainer = error.tone(90),
            .surface = neutral.tone(6),
            .surfaceDim = neutral.tone(6),
            .surfaceBright = neutral.tone(24),
            .surfaceContainerLowest = neutral.tone(4),
            .surfaceContainerLow = neutral.tone(10),
            .surfaceContainer = neutral.tone(12),
            .surfaceContainerHigh = neutral.tone(17),
            .surfaceContainerHighest = neutral.tone(22),
            .onSurface = neutral.tone(90),
            .surfaceVariant = neutral_variant.tone(30),
            .onSurfaceVariant = neutral_variant.tone(80),
            .surfaceTint = primary.tone(80),
            .outline = neutral_variant.tone(60),
            .outlineVariant = neutral_variant.tone(30),
            .shadow = neutral.tone(0),
            .scrim = neutral.tone(0),
            .inverseSurface = neutral.tone(90),
            .inverseOnSurface = neutral.tone(20),
            .inversePrimary = primary.tone(40),
        };
    }
} // namespace neko::style
