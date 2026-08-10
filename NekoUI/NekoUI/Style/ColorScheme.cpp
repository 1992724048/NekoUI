// 2026-08-10 06:16:49

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

            const float d = std::clamp(1.0F - 1.0F / 3.6F * std::exp((-la - 42.0F) / 92.0F), 0.0F, 1.0F);

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
            vc.aw = (2.0F * chromatic_adaptation(vc.rgb_d[0] * rw, fl) + chromatic_adaptation(vc.rgb_d[1] * gw, fl) + 0.05F * chromatic_adaptation(vc.rgb_d[2] * bw, fl)) * nbb;
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

            const float aw = (2.0F * ra + ga + 0.05F * ba) * vc.nbb;
            const float j = 100.0F * std::pow(aw / vc.aw, vc.c * vc.z);

            const float hue_rad = std::atan2(bb, a);
            const float e_t = 0.25F * (std::cos(hue_rad + 2.0F) + 3.8F);
            const float t = 50000.0F / 13.0F * vc.nc * vc.ncb * e_t * std::hypot(a, bb) / (ra + ga + 1.05F * ba + 0.305F);
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
            return type::Color{(static_cast<uint32_t>(r) << 24U) | (static_cast<uint32_t>(g) << 16U) | (static_cast<uint32_t>(b) << 8U) | 0xFFU,};
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
                const float lg = -0.9691453F * x + 1.8758853F * yy + 0.0415659F * z;
                const float lb = 0.0556209F * x - 0.2039552F * yy + 1.0571799F * z;

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
            }
            return false;
        }

        constexpr std::array kScaledDiscountFromLinrgb = {
            std::array{0.001200833568784504F, 0.002389694492170889F, 0.0002795742885861124F},
            std::array{0.0005891086651375999F, 0.0029785502573438758F, 0.0003270666104008398F},
            std::array{0.00010146692491640572F, 0.0005364214359186694F, 0.0032979401770712076F},
        };
        constexpr std::array kYFromLinrgb = {0.2126F, 0.7152F, 0.0722F};

        [[nodiscard]] auto critical_planes() -> const std::array<float, 255>& {
            static const auto kInstance = []() -> std::array<float, 255> {
                std::array<float, 255> planes{};
                for (std::size_t i = 0; i < planes.size(); ++i) {
                    const float sample = static_cast<float>(i) + 0.5F;
                    const float norm = sample / 255.0F;
                    const float lin = norm <= 0.040449936F ? norm / 12.92F : std::pow((norm + 0.055F) / 1.055F, 2.4F);
                    planes[i] = 100.0F * lin;
                }
                return planes;
            }();
            return kInstance;
        }

        [[nodiscard]] auto true_delinearized(const float rgb_component) -> float {
            const float normalized = rgb_component / 100.0F;
            return (normalized <= 0.0031308F ? normalized * 12.92F : 1.055F * std::pow(normalized, 1.0F / 2.4F) - 0.055F) * 255.0F;
        }

        [[nodiscard]] auto critical_plane_below(const float value) -> int {
            return static_cast<int>(std::floor(value - 0.5F));
        }

        [[nodiscard]] auto critical_plane_above(const float value) -> int {
            return static_cast<int>(std::ceil(value - 0.5F));
        }

        [[nodiscard]] auto sanitize_radians(const float angle) -> float {
            return std::fmod(angle + 8.0F * std::numbers::pi_v<float>, 2.0F * std::numbers::pi_v<float>);
        }

        [[nodiscard]] auto are_in_cyclic_order(const float first, const float second, const float third) -> bool {
            return sanitize_radians(second - first) < sanitize_radians(third - first);
        }

        [[nodiscard]] auto scaled_chromatic_adaptation(const float component) -> float {
            const float adapted = std::pow(std::fabs(component), 0.42F);
            return std::copysign(400.0F * adapted / (adapted + 27.13F), component);
        }

        [[nodiscard]] auto hue_of(const std::array<float, 3>& linrgb) -> float {
            const float scaled_r = linrgb[0] * kScaledDiscountFromLinrgb[0][0] + linrgb[1] * kScaledDiscountFromLinrgb[0][1] + linrgb[2] * kScaledDiscountFromLinrgb[0][2];
            const float scaled_g = linrgb[0] * kScaledDiscountFromLinrgb[1][0] + linrgb[1] * kScaledDiscountFromLinrgb[1][1] + linrgb[2] * kScaledDiscountFromLinrgb[1][2];
            const float scaled_b = linrgb[0] * kScaledDiscountFromLinrgb[2][0] + linrgb[1] * kScaledDiscountFromLinrgb[2][1] + linrgb[2] * kScaledDiscountFromLinrgb[2][2];

            const float adapt_r = scaled_chromatic_adaptation(scaled_r);
            const float adapt_g = scaled_chromatic_adaptation(scaled_g);
            const float adapt_b = scaled_chromatic_adaptation(scaled_b);

            const float opponent_a = (11.0F * adapt_r - 12.0F * adapt_g + adapt_b) / 11.0F;
            const float opponent_b = (adapt_r + adapt_g - 2.0F * adapt_b) / 9.0F;
            return std::atan2(opponent_b, opponent_a);
        }

        [[nodiscard]] constexpr auto is_bounded(const float value) -> bool {
            return 0.0F <= value && value <= 100.0F;
        }

        [[nodiscard]] auto nth_vertex(const float plane_y, const int vertex_index) -> std::array<float, 3> {
            constexpr std::array kInvalid{-1.0F, -1.0F, -1.0F};
            const float coord_a = vertex_index % 4 <= 1 ? 0.0F : 100.0F;
            const float coord_b = vertex_index % 2 == 0 ? 0.0F : 100.0F;

            if (vertex_index < 4) {
                const float green = coord_a;
                const float blue = coord_b;
                const float red = (plane_y - green * kYFromLinrgb[1] - blue * kYFromLinrgb[2]) / kYFromLinrgb[0];
                return is_bounded(red) ? std::array{red, green, blue} : kInvalid;
            }
            if (vertex_index < 8) {
                const float blue = coord_a;
                const float red = coord_b;
                const float green = (plane_y - red * kYFromLinrgb[0] - blue * kYFromLinrgb[2]) / kYFromLinrgb[1];
                return is_bounded(green) ? std::array{red, green, blue} : kInvalid;
            }
            const float red = coord_a;
            const float green = coord_b;
            const float blue = (plane_y - red * kYFromLinrgb[0] - green * kYFromLinrgb[1]) / kYFromLinrgb[2];
            return is_bounded(blue) ? std::array{red, green, blue} : kInvalid;
        }

        [[nodiscard]] auto intercept(const float source, const float mid, const float target) -> float {
            return (mid - source) / (target - source);
        }

        [[nodiscard]] auto lerp_point(const std::array<float, 3>& source, const float factor, const std::array<float, 3>& target) -> std::array<float, 3> {
            return {source[0] + (target[0] - source[0]) * factor, source[1] + (target[1] - source[1]) * factor, source[2] + (target[2] - source[2]) * factor,};
        }

        [[nodiscard]] auto set_coordinate(const std::array<float, 3>& source, const float coordinate, const std::array<float, 3>& target, const int axis) -> std::array<float, 3> {
            return lerp_point(source, intercept(source[axis], coordinate, target[axis]), target);
        }

        [[nodiscard]] auto midpoint(const std::array<float, 3>& point_a, const std::array<float, 3>& point_b) -> std::array<float, 3> {
            return {(point_a[0] + point_b[0]) * 0.5F, (point_a[1] + point_b[1]) * 0.5F, (point_a[2] + point_b[2]) * 0.5F};
        }

        [[nodiscard]] auto bisect_to_segment(const float plane_y, const float target_hue) -> std::array<std::array<float, 3>, 2> {
            std::array left{-1.0F, -1.0F, -1.0F};
            std::array<float, 3> right = left;
            float left_hue = 0.0F;
            float right_hue = 0.0F;
            bool initialized = false;
            bool uncut = true;

            for (int vertex_index = 0; vertex_index < 12; ++vertex_index) {
                const std::array<float, 3> mid = nth_vertex(plane_y, vertex_index);
                if (mid[0] < 0.0F) {
                    continue;
                }
                const float mid_hue = hue_of(mid);
                if (!initialized) {
                    left = mid;
                    right = mid;
                    left_hue = mid_hue;
                    right_hue = mid_hue;
                    initialized = true;
                    continue;
                }
                if (uncut || are_in_cyclic_order(left_hue, mid_hue, right_hue)) {
                    uncut = false;
                    if (are_in_cyclic_order(left_hue, target_hue, mid_hue)) {
                        right = mid;
                        right_hue = mid_hue;
                    } else {
                        left = mid;
                        left_hue = mid_hue;
                    }
                }
            }
            return {left, right};
        }

        [[nodiscard]] auto bisect_to_limit(const float plane_y, const float target_hue) -> std::array<float, 3> {
            const std::array<std::array<float, 3>, 2> segment = bisect_to_segment(plane_y, target_hue);
            std::array<float, 3> left = segment[0];
            float left_hue = hue_of(left);
            std::array<float, 3> right = segment[1];

            for (int axis = 0; axis < 3; ++axis) {
                if (left[axis] == right[axis]) {
                    continue;
                }
                int l_plane = -1;
                int r_plane = 255;
                if (left[axis] < right[axis]) {
                    l_plane = critical_plane_below(true_delinearized(left[axis]));
                    r_plane = critical_plane_above(true_delinearized(right[axis]));
                } else {
                    l_plane = critical_plane_above(true_delinearized(left[axis]));
                    r_plane = critical_plane_below(true_delinearized(right[axis]));
                }
                for (int i = 0; i < 8; ++i) {
                    if (std::abs(r_plane - l_plane) <= 1) {
                        break;
                    }
                    const int m_plane = static_cast<int>(std::floor(static_cast<float>(l_plane + r_plane) * 0.5F));
                    const float mid_plane_coordinate = critical_planes()[static_cast<std::size_t>(m_plane)];
                    const std::array<float, 3> mid = set_coordinate(left, mid_plane_coordinate, right, axis);
                    const float mid_hue = hue_of(mid);
                    if (are_in_cyclic_order(left_hue, target_hue, mid_hue)) {
                        right = mid;
                        r_plane = m_plane;
                    } else {
                        left = mid;
                        left_hue = mid_hue;
                        l_plane = m_plane;
                    }
                }
            }
            return midpoint(left, right);
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

            auto lin = std::array{y, y, y};
            if (!find_linear_rgb(hue, hct.chroma, y, lin)) {
                lin = bisect_to_limit(y, hue * std::numbers::pi_v<float> / 180.0F);
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
            .on_primary = primary.tone(100),
            .primary_container = primary.tone(90),
            .on_primary_container = primary.tone(30),
            .primary_fixed = primary.tone(90),
            .primary_fixed_dim = primary.tone(80),
            .on_primary_fixed = primary.tone(10),
            .on_primary_fixed_variant = primary.tone(30),
            .secondary = secondary.tone(40),
            .on_secondary = secondary.tone(100),
            .secondary_container = secondary.tone(90),
            .on_secondary_container = secondary.tone(30),
            .secondary_fixed = secondary.tone(90),
            .secondary_fixed_dim = secondary.tone(80),
            .on_secondary_fixed = secondary.tone(10),
            .on_secondary_fixed_variant = secondary.tone(30),
            .tertiary = tertiary.tone(40),
            .on_tertiary = tertiary.tone(100),
            .tertiary_container = tertiary.tone(90),
            .on_tertiary_container = tertiary.tone(30),
            .tertiary_fixed = tertiary.tone(90),
            .tertiary_fixed_dim = tertiary.tone(80),
            .on_tertiary_fixed = tertiary.tone(10),
            .on_tertiary_fixed_variant = tertiary.tone(30),
            .error = error.tone(40),
            .on_error = error.tone(100),
            .error_container = error.tone(90),
            .on_error_container = error.tone(30),
            .surface = neutral.tone(98),
            .surface_dim = neutral.tone(87),
            .surface_bright = neutral.tone(98),
            .surface_container_lowest = neutral.tone(100),
            .surface_container_low = neutral.tone(96),
            .surface_container = neutral.tone(94),
            .surface_container_high = neutral.tone(92),
            .surface_container_highest = neutral.tone(90),
            .on_surface = neutral.tone(10),
            .surface_variant = neutral_variant.tone(90),
            .on_surface_variant = neutral_variant.tone(30),
            .surface_tint = primary.tone(40),
            .outline = neutral_variant.tone(50),
            .outline_variant = neutral_variant.tone(80),
            .shadow = neutral.tone(0),
            .scrim = neutral.tone(0),
            .inverse_surface = neutral.tone(20),
            .inverse_on_surface = neutral.tone(95),
            .inverse_primary = primary.tone(80),
        };
    }

    auto ColorScheme::dark(const type::Color seed) -> ColorScheme {
        const auto [primary, secondary, tertiary, neutral, neutral_variant, error] = make_palettes(seed);
        return {
            .brightness = Brightness::Dark,
            .primary = primary.tone(80),
            .on_primary = primary.tone(20),
            .primary_container = primary.tone(30),
            .on_primary_container = primary.tone(90),
            .primary_fixed = primary.tone(90),
            .primary_fixed_dim = primary.tone(80),
            .on_primary_fixed = primary.tone(10),
            .on_primary_fixed_variant = primary.tone(30),
            .secondary = secondary.tone(80),
            .on_secondary = secondary.tone(20),
            .secondary_container = secondary.tone(30),
            .on_secondary_container = secondary.tone(90),
            .secondary_fixed = secondary.tone(90),
            .secondary_fixed_dim = secondary.tone(80),
            .on_secondary_fixed = secondary.tone(10),
            .on_secondary_fixed_variant = secondary.tone(30),
            .tertiary = tertiary.tone(80),
            .on_tertiary = tertiary.tone(20),
            .tertiary_container = tertiary.tone(30),
            .on_tertiary_container = tertiary.tone(90),
            .tertiary_fixed = tertiary.tone(90),
            .tertiary_fixed_dim = tertiary.tone(80),
            .on_tertiary_fixed = tertiary.tone(10),
            .on_tertiary_fixed_variant = tertiary.tone(30),
            .error = error.tone(80),
            .on_error = error.tone(20),
            .error_container = error.tone(30),
            .on_error_container = error.tone(90),
            .surface = neutral.tone(6),
            .surface_dim = neutral.tone(6),
            .surface_bright = neutral.tone(24),
            .surface_container_lowest = neutral.tone(4),
            .surface_container_low = neutral.tone(10),
            .surface_container = neutral.tone(12),
            .surface_container_high = neutral.tone(17),
            .surface_container_highest = neutral.tone(22),
            .on_surface = neutral.tone(90),
            .surface_variant = neutral_variant.tone(30),
            .on_surface_variant = neutral_variant.tone(80),
            .surface_tint = primary.tone(80),
            .outline = neutral_variant.tone(60),
            .outline_variant = neutral_variant.tone(30),
            .shadow = neutral.tone(0),
            .scrim = neutral.tone(0),
            .inverse_surface = neutral.tone(90),
            .inverse_on_surface = neutral.tone(20),
            .inverse_primary = primary.tone(40),
        };
    }
} // namespace neko::style
