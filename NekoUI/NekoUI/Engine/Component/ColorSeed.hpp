// 2026-07-08 18:13:08

#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <glm/glm.hpp>
#include "../../Type.hpp"

namespace neko::color {
    struct ColorScheme; // forward decl

    namespace detail {
        // ARGB Utility
        constexpr auto argb_from_rgb(const int r, const int g, const int b) -> std::int32_t {
            return (255 << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
        }

        constexpr auto alpha_from_argb(const std::int32_t a) -> int {
            return (a >> 24) & 0xFF;
        }

        constexpr auto red_from_argb(const std::int32_t a) -> int {
            return (a >> 16) & 0xFF;
        }

        constexpr auto green_from_argb(const std::int32_t a) -> int {
            return (a >> 8) & 0xFF;
        }

        constexpr auto blue_from_argb(const std::int32_t a) -> int {
            return a & 0xFF;
        }

        using Argb = std::int32_t;

        constexpr auto argb_to_color(const Argb argb) -> type::Color {
            return {red_from_argb(argb) / 255.0F, green_from_argb(argb) / 255.0F, blue_from_argb(argb) / 255.0F, alpha_from_argb(argb) / 255.0F};
        }

        // sRGB Gamma
        inline auto linearized(const int rgb_component) -> double {
            const double n = rgb_component / 255.0;
            return n <= 0.040449936 ? n / 12.92 : std::pow((n + 0.055) / 1.055, 2.4);
        }

        inline auto delinearized(const double rgb_component) -> int {
            double n;
            if (rgb_component <= 0.0031308) {
                n = rgb_component * 12.92;
            } else {
                n = 1.055 * std::pow(rgb_component, 1.0 / 2.4) - 0.055;
            }
            return static_cast<int>(std::round(n * 255.0));
        }

        inline auto argb_from_linrgb(const double r, const double g, const double b) -> std::int32_t {
            return argb_from_rgb(delinearized(r), delinearized(g), delinearized(b));
        }

        // L* ↔ Y (CIE Lightness ↔ Relative Luminance)
        inline auto y_from_lstar(const double lstar) -> double {
            return lstar > 8.0 ? std::pow((lstar + 16.0) / 116.0, 3.0) : lstar / 903.3;
        }

        inline auto lstar_from_y(const double y) -> double {
            return y > 0.008856 ? 116.0 * std::cbrt(y) - 16.0 : y * 903.3;
        }

        inline auto lstar_from_argb(const std::int32_t argb) -> double {
            const auto y = 0.2126 * linearized(red_from_argb(argb)) + 0.7152 * linearized(green_from_argb(argb)) + 0.0722 * linearized(blue_from_argb(argb));
            return lstar_from_y(y);
        }

        inline auto argb_from_lstar(const double lstar) -> std::int32_t {
            const auto c = delinearized(y_from_lstar(lstar));
            return argb_from_rgb(c, c, c);
        }

        // Math Helpers
        inline auto sanitize_degrees(double d) -> double {
            d = std::fmod(d, 360.0);
            return d < 0 ? d + 360.0 : d;
        }

        // Signum function: -1 for negative, 0 for zero, 1 for positive
        inline auto signum(const double x) -> double {
            return x < 0.0 ? -1.0 : x > 0.0 ? 1.0 : 0.0;
        }

        // Chromatic adaptation: non-linear compression of cone responses
        inline auto chromatic_adaptation(const double component) -> double {
            const double af = std::pow(std::abs(component), 0.42);
            return signum(component) * 400.0 * af / (af + 27.13);
        }

        // ViewingConditions (CAM16 viewing conditions for D65)
        // Ported from material_color_utilities viewing_conditions.ts / viewing_conditions.dart
        struct ViewingConditions {
            double n = 0.0, aw = 0.0, nbb = 0.0, ncb = 0.0;
            double c = 0.0, nc = 0.0, fl = 0.0, fLRoot = 0.0, z = 0.0;

            static auto make() -> ViewingConditions {
                // Default D65 white point (CIE standard illuminant D65)
                constexpr std::array whitePoint{0.95047, 1.0, 1.08883};

                // adaptingLuminance: standard sRGB-like conditions
                // Default = (200/π) * Y_from_L*(50) / 100
                const double adaptingLuminance = 200.0 / std::numbers::pi * y_from_lstar(50.0) / 100.0;

                constexpr double backgroundLstar = 50.0;
                constexpr double surround = 2.0; // average surround

                // Transform white point XYZ to CAT02 cone responses
                constexpr double rW = whitePoint[0] * 0.401288 + whitePoint[1] * 0.650173 + whitePoint[2] * -0.051461;
                constexpr double gW = whitePoint[0] * -0.250268 + whitePoint[1] * 1.204414 + whitePoint[2] * 0.045854;
                constexpr double bW = whitePoint[0] * -0.002079 + whitePoint[1] * 0.048952 + whitePoint[2] * 0.953127;

                // Surround → CAM16 surround factor f (0.8–1.0)
                constexpr double f = 0.8 + surround / 10.0;

                // CAM16 exponential non-linearity c
                constexpr double c = f >= 0.9 ? 0.59 + (0.69 - 0.59) * ((f - 0.9) * 10.0) : 0.59;

                // Degree of adaptation d
                double d = f * (1.0 - 1.0 / 3.6 * std::exp((-adaptingLuminance - 42.0) / 92.0));
                d = std::clamp(d, 0.0, 1.0);

                // Chromatic induction factor nc = f (surround factor)
                constexpr double nc = f;

                // Discounted cone responses to white point
                // Note: uses 100.0 (luminance of reference white), NOT whitePoint[1].
                const std::array rgbD = {d * (100.0 / rW) + 1.0 - d, d * (100.0 / gW) + 1.0 - d, d * (100.0 / bW) + 1.0 - d,};

                // k factor for luminance-level adaptation
                const double k = 1.0 / (5.0 * adaptingLuminance + 1.0);
                const double k4 = k * k * k * k;
                const double k4F = 1.0 - k4;

                // Luminance-level adaptation factor fl
                const double fl = k4 * 5.0 * adaptingLuminance + 0.1 * k4F * k4F * std::pow(5.0 * adaptingLuminance, 1.0 / 3.0);
                const double fLRoot = std::pow(fl, 0.25);

                // n: ratio of background relative luminance to white relative luminance
                const double n = y_from_lstar(backgroundLstar) / whitePoint[1];

                // Base exponential nonlinearity z
                const double z = 1.48 + std::sqrt(n);

                // Luminance-level induction factors
                const double nbb = 0.725 / std::pow(n, 0.2);
                const double ncb = nbb;

                // Non-linear cone responses for white point (with fl adaptation)
                const std::array rgbAFactors = {std::pow(fl * rgbD[0] * rW / 100.0, 0.42), std::pow(fl * rgbD[1] * gW / 100.0, 0.42), std::pow(fl * rgbD[2] * bW / 100.0, 0.42),};

                const std::array rgbA = {400.0 * rgbAFactors[0] / (rgbAFactors[0] + 27.13), 400.0 * rgbAFactors[1] / (rgbAFactors[1] + 27.13), 400.0 * rgbAFactors[2] / (rgbAFactors[2] + 27.13),};

                // Achromatic response for white
                const double aw = (40.0 * rgbA[0] + 20.0 * rgbA[1] + rgbA[2]) / 20.0 * nbb;

                return {.n = n, .aw = aw, .nbb = nbb, .ncb = ncb, .c = c, .nc = nc, .fl = fl, .fLRoot = fLRoot, .z = z};
            }

            static ViewingConditions DEFAULT;
        };

        inline ViewingConditions ViewingConditions::DEFAULT = make();

        // CAM16 forward transform
        struct Cam16 {
            double hue = 0.0, chroma = 0.0, j = 0.0;

            static auto from_argb(const std::int32_t argb) -> Cam16 {
                const auto rL = linearized(red_from_argb(argb));
                const auto gL = linearized(green_from_argb(argb));
                const auto bL = linearized(blue_from_argb(argb));

                const auto x = 0.41233895 * rL + 0.35762064 * gL + 0.18051042 * bL;
                const auto y = 0.2126 * rL + 0.7152 * gL + 0.0722 * bL;
                const auto zV = 0.01932141 * rL + 0.11916382 * gL + 0.95034478 * bL;

                const auto& vc = ViewingConditions::DEFAULT;
                const double rC = x * 0.401288 + y * 0.650173 + zV * -0.051461;
                const double gC = x * -0.250268 + y * 1.204414 + zV * 0.045854;
                const double bC = x * -0.002079 + y * 0.048952 + zV * 0.953127;

                // Apply cone response adaptation (D65 adapting white → identity)
                const double rD = rC;
                const double gD = gC;
                const double bD = bC;

                // Non-linear compression via CAM16 chromatic adaptation
                const double rA = chromatic_adaptation(rD / 100.0);
                const double gA = chromatic_adaptation(gD / 100.0);
                const double bA = chromatic_adaptation(bD / 100.0);

                // Achromatic response a
                const double a = vc.nbb * (2.0 * rA + gA + 0.05 * bA);
                const double j = 100.0 * std::pow(a / vc.aw, vc.c * vc.z);

                // Hue
                const double hRad = std::atan2(bA - gA, rA - gA);
                double hDeg = hRad * 180.0 / std::numbers::pi;
                if (hDeg < 0) {
                    hDeg += 360.0;
                }

                const double hRadians = hDeg * std::numbers::pi / 180.0;
                const double eT = 0.25 * (std::cos(hRadians + 2.0) + 3.8);

                // Chroma
                const double tBase = 50000.0 / 13.0 * vc.nc * vc.ncb * eT;
                const double tNum = std::sqrt(rA * rA + gA * gA - 2.0 * rA * gA - rA * bA + gA * bA);
                const double tDen = rA + gA + 1.05 * bA + 0.305;
                const double t = tBase * tNum / tDen;

                const double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
                const double chroma = alpha * std::sqrt(j / 100.0);

                return {.hue = hDeg, .chroma = chroma, .j = j};
            }
        };

        // HCT Solver (Newton Iteration)
        // Ported from material_color_utilities hct_solver.dart / hct_solver.ts
        // See: https://github.com/material-foundation/material-color-utilities
        struct HctSolver {
            static constexpr int NUM_ITERATIONS = 5;

            // Matrix Constants
            // SCALED_DISCOUNT_FROM_LINRGB: Maps linear RGB to scaled discount space.
            // Embeds the CAT02 chromatic adaptation transform with D65 illuminant
            // discounting baked in, allowing hueOf() to work directly on linear RGB.
            static constexpr std::array<std::array<double, 3>, 3> SCALED_DISCOUNT_FROM_LINRGB{
                {
                    {0.001200833568784504, 0.002389694492170889, 0.0002795742885861124},
                    {0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398},
                    {0.00010146692491640572, 0.0005364214359186694, 0.0032979401770712076},
                }
            };

            // LINRGB_FROM_SCALED_DISCOUNT: Inverse of SCALED_DISCOUNT_FROM_LINRGB.
            // Used in findResultByJ to recover linear RGB from adapted cone responses.
            static constexpr std::array<std::array<double, 3>, 3> LINRGB_FROM_SCALED_DISCOUNT{
                {
                    {1373.2198709594231, -1100.4251190754821, -7.278681089101213},
                    {-271.815969077903, 559.6580465940733, -32.46047482791194},
                    {1.9622899599665666, -57.173814538844006, 308.7233197812385},
                }
            };

            // Y_FROM_LINRGB: Coefficients for computing Y (relative luminance) from
            // linear RGB: Y = 0.2126*R + 0.7152*G + 0.0722*B
            static constexpr std::array<double, 3> Y_FROM_LINRGB = {0.2126, 0.7152, 0.0722};

            // CRITICAL_PLANES: Pre-computed sRGB transfer function values at critical
            // points used by bisectToLimit for binary search on the gamut boundary.
            // 255 entries correspond to the midpoints between [0, 255] quantization levels.
            static constexpr std::array<double, 255> CRITICAL_PLANES{
                0.015176349177441876,
                0.045529047532325624,
                0.07588174588720938,
                0.10623444424209313,
                0.13658714259697685,
                0.16693984095186062,
                0.19729253930674434,
                0.2276452376616281,
                0.2579979360165119,
                0.28835063437139563,
                std::numbers::inv_pi,
                0.350925934958123,
                0.3848314933096426,
                0.42057480301049466,
                0.458183274052838,
                0.4976837250274023,
                0.5391024159806381,
                0.5824650784040898,
                0.6277969426914107,
                0.6751227633498623,
                0.7244668422128921,
                0.775853049866786,
                0.829304845476233,
                0.8848452951698498,
                0.942497089126609,
                1.0022825574869039,
                1.0642236851973577,
                1.1283421258858297,
                1.1946592148522128,
                1.2631959812511864,
                1.3339731595349034,
                1.407011200216447,
                1.4823302800086415,
                1.5599503113873272,
                1.6398909516233677,
                1.7221716113234105,
                1.8068114625156377,
                1.8938294463134073,
                1.9832442801866852,
                2.075074464868551,
                2.1693382909216234,
                2.2660538449872063,
                2.36523901573795,
                2.4669114995532007,
                2.5710888059345764,
                2.6777882626779785,
                2.7870270208169257,
                2.898822059350997,
                3.0131901897720907,
                3.1301480604002863,
                3.2497121605402226,
                3.3718988244681087,
                3.4967242352587946,
                3.624204428461639,
                3.754355295633311,
                3.887192587735158,
                4.022731918402185,
                4.160988767090289,
                4.301978482107941,
                4.445716283538092,
                4.592217266055746,
                4.741496401646282,
                4.893568542229298,
                5.048448422192488,
                5.20615066083972,
                5.3666897647573375,
                5.5300801301023865,
                5.696336044816294,
                5.865471690767354,
                6.037501145825082,
                6.212438385869475,
                6.390297286737924,
                6.571091626112461,
                6.7548350853498045,
                6.941541251256611,
                7.131223617812143,
                7.323895587840543,
                7.5195704746346665,
                7.7182615035334345,
                7.919981813454504,
                8.124744458384042,
                8.332562408825165,
                8.543448553206703,
                8.757415699253682,
                8.974476575321063,
                9.194643831691977,
                9.417930041841839,
                9.644347703669503,
                9.873909240696694,
                10.106627003236781,
                10.342513269534024,
                10.58158024687427,
                10.8238400726681,
                11.069304815507364,
                11.317986476196008,
                11.569896988756009,
                11.825048221409341,
                12.083451977536606,
                12.345119996613247,
                12.610063955123938,
                12.878295467455942,
                13.149826086772048,
                13.42466730586372,
                13.702830557985108,
                13.984327217668513,
                14.269168601521828,
                14.55736596900856,
                14.848930523210871,
                15.143873411576273,
                15.44220572664832,
                15.743938506781891,
                16.04908273684337,
                16.35764934889634,
                16.66964922287304,
                16.985093187232053,
                17.30399201960269,
                17.62635644741625,
                17.95219714852476,
                18.281524751807332,
                18.614349837764564,
                18.95068293910138,
                19.290534541298456,
                19.633915083172692,
                19.98083495742689,
                20.331304511189067,
                20.685334046541502,
                21.042933821039977,
                21.404114048223256,
                21.76888489811322,
                22.137256497705877,
                22.50923893145328,
                22.884842241736916,
                23.264076429332462,
                23.6469514538663,
                24.033477234264016,
                24.42366364919083,
                24.817520537484558,
                25.21505769858089,
                25.61628489293138,
                26.021211842414342,
                26.429848230738664,
                26.842203703840827,
                27.258287870275353,
                27.678110301598522,
                28.10168053274597,
                28.529008062403893,
                28.96010235337422,
                29.39497283293396,
                29.83362889318845,
                30.276079891419332,
                30.722335150426627,
                31.172403958865512,
                31.62629557157785,
                32.08401920991837,
                32.54558406207592,
                33.010999283389665,
                33.4802739966603,
                33.953417292456834,
                34.430438229418264,
                34.911345834551085,
                35.39614910352207,
                35.88485700094671,
                36.37747846067349,
                36.87402238606382,
                37.37449765026789,
                37.87891309649659,
                38.38727753828926,
                38.89959975977785,
                39.41588851594697,
                39.93615253289054,
                40.460400508064545,
                40.98864111053629,
                41.520882981230194,
                42.05713473317016,
                42.597404951718396,
                43.141702194811224,
                43.6900349931913,
                44.24241185063697,
                44.798841244188324,
                45.35933162437017,
                45.92389141541209,
                46.49252901546552,
                47.065252796817916,
                47.64207110610409,
                48.22299226451468,
                48.808024568002054,
                49.3971762874833,
                49.9904556690408,
                50.587870934119984,
                51.189430279724725,
                51.79514187861014,
                52.40501387947288,
                53.0190544071392,
                53.637271562750364,
                54.259673423945976,
                54.88626804504493,
                55.517063457223934,
                56.15206766869424,
                56.79128866487574,
                57.43473440856916,
                58.08241284012621,
                58.734331877617365,
                59.39049941699807,
                60.05092333227251,
                60.715611475655585,
                61.38457167773311,
                62.057811747619894,
                62.7353394731159,
                63.417162620860914,
                64.10328893648692,
                64.79372614476921,
                65.48848194977529,
                66.18756403501224,
                66.89098006357258,
                67.59873767827808,
                68.31084450182222,
                69.02730813691093,
                69.74813616640164,
                70.47333615344107,
                71.20291564160104,
                71.93688215501312,
                72.67524319850172,
                73.41800625771542,
                74.16517879925733,
                74.9167682708136,
                75.67278210128072,
                76.43322770089146,
                77.1981124613393,
                77.96744375590167,
                78.74122893956174,
                79.51947534912904,
                80.30219030335869,
                81.08938110306934,
                81.88105503125999,
                82.67721935322541,
                83.4778813166706,
                84.28304815182372,
                85.09272707154808,
                85.90692527145302,
                86.72564993000343,
                87.54890820862819,
                88.3767072518277,
                89.2090541872801,
                90.04595612594655,
                90.88742016217518,
                91.73345337380438,
                92.58406282226491,
                93.43925555268066,
                94.29903859396902,
                95.16341895893969,
                96.03240364439274,
                96.9059996312159,
                97.78421388448044,
                98.6670533535366,
                99.55452497210776,
            };

            // Public Entry Points

            // Solve for an ARGB color given hue (degrees), chroma, and L* (tone).
            // If the desired chroma is unreachable at this hue+tone, returns the
            // closest reachable color on the sRGB gamut boundary.
            static auto solve_to_int(double hue_deg, const double chroma, const double lstar) -> std::int32_t {
                if (chroma < 0.0001 || lstar < 0.0001 || lstar > 99.9999) {
                    return argb_from_lstar(lstar);
                }
                hue_deg = sanitize_degrees(hue_deg);
                const double hueRadians = hue_deg / 180.0 * std::numbers::pi;
                const double y = y_from_lstar(lstar);
                if (const std::int32_t exactAnswer = find_result_by_j(hueRadians, chroma, y); exactAnswer != 0) {
                    return exactAnswer;
                }
                // Chroma was unreachable, fall back to the gamut boundary at this hue+Y
                const std::array<double, 3> linrgb = bisect_to_limit(y, hueRadians);
                return argb_from_linrgb100(linrgb[0], linrgb[1], linrgb[2]);
            }

            // Solve for a Cam16 color. Convenience wrapper.
            static auto solve_to_cam(const double hue_deg, const double chroma, const double lstar) -> Cam16 {
                return Cam16::from_argb(solve_to_int(hue_deg, chroma, lstar));
            }
        private:
            // Private Helpers

            static auto sanitize_radians(const double angle) -> double {
                return std::fmod(angle + 8.0 * std::numbers::pi, 2.0 * std::numbers::pi);
            }

            // Matrix multiply: 1x3 row vector x 3x3 matrix -> 3-element vector
            static auto matrix_multiply(const std::array<double, 3>& row, const std::array<std::array<double, 3>, 3>& matrix) -> std::array<double, 3> {
                return std::array{
                    row[0] * matrix[0][0] + row[1] * matrix[0][1] + row[2] * matrix[0][2],
                    row[0] * matrix[1][0] + row[1] * matrix[1][1] + row[2] * matrix[1][2],
                    row[0] * matrix[2][0] + row[1] * matrix[2][1] + row[2] * matrix[2][2],
                };
            }

            // Forward/Inverse Chromatic Adaptation
            // These implement the nonlinear chromatic adaptation from CAM16.
            // Forward: linear cone response -> adapted response (using 0.42 power)
            // Inverse: adapted response -> linear cone response

            static auto inverse_chromatic_adaptation(const double adapted) -> double {
                const double adaptedAbs = std::abs(adapted);
                const double base = std::max(0.0, 27.13 * adaptedAbs / (400.0 - adaptedAbs));
                return signum(adapted) * std::pow(base, 1.0 / 0.42);
            }

            // sRGB Transfer Helpers (for [0, 100] linear RGB range)

            // Delinearize: [0, 100] linear -> [0, 255] float (no clamping).
            // Used only in bisectToLimit for critical plane computation.
            static auto true_delinearized(const double rgb_component) -> double {
                const double normalized = rgb_component / 100.0;
                double d;
                if (normalized <= 0.0031308) {
                    d = normalized * 12.92;
                } else {
                    d = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
                }
                return d * 255.0;
            }

            // Delinearize: [0, 100] linear -> [0, 255] integer (with clamping).
            // Used for final ARGB conversion from linear RGB.
            static auto delinearized_int(const double rgb_component) -> int {
                const double normalized = rgb_component / 100.0;
                double d;
                if (normalized <= 0.0031308) {
                    d = normalized * 12.92;
                } else {
                    d = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
                }
                return std::clamp(static_cast<int>(std::round(d * 255.0)), 0, 255);
            }

            // ARGB from [0, 100] linear RGB values (MCU convention).
            static auto argb_from_linrgb100(const double r, const double g, const double b) -> std::int32_t {
                return argb_from_rgb(delinearized_int(r), delinearized_int(g), delinearized_int(b));
            }

            // RGB Cube Boundary Utilities (for bisectToLimit)

            // Compute CAM16 hue in radians from linear RGB.
            // Transforms via SCALED_DISCOUNT_FROM_LINRGB then applies chromatic adaptation.
            static auto hue_of(const std::array<double, 3>& linrgb) -> double {
                const auto scaled = matrix_multiply(linrgb, SCALED_DISCOUNT_FROM_LINRGB);
                const double rA = chromatic_adaptation(scaled[0]);
                const double gA = chromatic_adaptation(scaled[1]);
                const double bA = chromatic_adaptation(scaled[2]);
                // Redness-greenness
                const double a = (11.0 * rA + -12.0 * gA + bA) / 11.0;
                // Yellowness-blueness
                const double b = (rA + gA - 2.0 * bA) / 9.0;
                return std::atan2(b, a);
            }

            // Check if angles a,b,c are in cyclic order on the circle.
            static auto are_in_cyclic_order(const double a, const double b, const double c) -> bool {
                const double deltaAB = sanitize_radians(b - a);
                const double deltaAC = sanitize_radians(c - a);
                return deltaAB < deltaAC;
            }

            // Solve lerp parameter t: lerp(source, target, t) = mid
            static auto intercept(const double source, const double mid, const double target) -> double {
                return (mid - source) / (target - source);
            }

            // Linear interpolation between two 3D points
            static auto lerp_point(const std::array<double, 3>& source, const double t, const std::array<double, 3>& target) -> std::array<double, 3> {
                return std::array{source[0] + (target[0] - source[0]) * t, source[1] + (target[1] - source[1]) * t, source[2] + (target[2] - source[2]) * t,};
            }

            // Intersect segment AB with plane R=coord, G=coord, or B=coord.
            static auto set_coordinate(const std::array<double, 3>& source, const double coordinate, const std::array<double, 3>& target, const int axis) -> std::array<double, 3> {
                const double t = intercept(source[axis], coordinate, target[axis]);
                return lerp_point(source, t, target);
            }

            // Check if x is in [0, 100] (valid linear RGB range)
            static auto is_bounded(const double x) -> bool {
                return 0.0 <= x && x <= 100.0;
            }

            // Compute the nth vertex of the polygonal intersection of Y-plane with RGB cube.
            // Returns {-1, -1, -1} if the vertex lies outside the cube.
            // n in [0, 11]: 4 vertices per face x 3 faces (R, G, B free).
            static auto nth_vertex(const double y, const int n) -> std::array<double, 3> {
                constexpr double kR = Y_FROM_LINRGB[0];
                constexpr double kG = Y_FROM_LINRGB[1];
                constexpr double kB = Y_FROM_LINRGB[2];
                const double coordA = n % 4 <= 1 ? 0.0 : 100.0;
                const double coordB = n % 2 == 0 ? 0.0 : 100.0;

                if (n < 4) {
                    const double g = coordA;
                    const double b = coordB;
                    const double r = (y - g * kG - b * kB) / kR;
                    if (is_bounded(r)) {
                        return {r, g, b};
                    }
                } else if (n < 8) {
                    const double b = coordA;
                    const double r = coordB;
                    const double g = (y - r * kR - b * kB) / kG;
                    if (is_bounded(g)) {
                        return {r, g, b};
                    }
                } else {
                    const double r = coordA;
                    const double g = coordB;
                    const double b = (y - r * kR - g * kG) / kB;
                    if (is_bounded(b)) {
                        return {r, g, b};
                    }
                }
                return {-1.0, -1.0, -1.0};
            }

            // Find the two vertices on the Y-plane polygon that bracket the target hue.
            // Returns via output parameters.
            static auto bisect_to_segment(const double y, const double target_hue, std::array<double, 3>& out_left, std::array<double, 3>& out_right) -> void {
                std::array left{-1.0, -1.0, -1.0};
                std::array<double, 3> right = left;
                double left_hue = 0.0;
                double right_hue = 0.0;
                bool initialized = false;
                bool uncut = true;

                for (int n = 0; n < 12; n++) {
                    auto mid = nth_vertex(y, n);
                    if (mid[0] < 0) {
                        continue;
                    }
                    const double midHue = hue_of(mid);
                    if (!initialized) {
                        left = mid;
                        right = mid;
                        left_hue = midHue;
                        right_hue = midHue;
                        initialized = true;
                        continue;
                    }
                    if (uncut || are_in_cyclic_order(left_hue, midHue, right_hue)) {
                        uncut = false;
                        if (are_in_cyclic_order(left_hue, target_hue, midHue)) {
                            right = mid;
                            right_hue = midHue;
                        } else {
                            left = mid;
                            left_hue = midHue;
                        }
                    }
                }
                out_left = left;
                out_right = right;
            }

            // Midpoint of two 3D points.
            static auto midpoint(const std::array<double, 3>& a, const std::array<double, 3>& b) -> std::array<double, 3> {
                return std::array{(a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0, (a[2] + b[2]) / 2.0,};
            }

            // Critical plane index helpers: map delinearized (0-255) to CRITICAL_PLANES indices.
            static auto critical_plane_below(const double x) -> int {
                return static_cast<int>(std::floor(x - 0.5));
            }

            static auto critical_plane_above(const double x) -> int {
                return static_cast<int>(std::ceil(x - 0.5));
            }

            // Gamut Boundary Search
            // Given Y and target hue, find the point on the sRGB gamut boundary
            // that has that Y and hue. Returns linear RGB in [0, 100] range.

            static auto bisect_to_limit(const double y, const double target_hue) -> std::array<double, 3> {
                std::array<double, 3> left{};
                std::array<double, 3> right{};
                bisect_to_segment(y, target_hue, left, right);

                double left_hue = hue_of(left);

                for (int axis = 0; axis < 3; axis++) {
                    if (left[axis] != right[axis]) {
                        int l_plane;
                        int r_plane;
                        if (left[axis] < right[axis]) {
                            l_plane = critical_plane_below(true_delinearized(left[axis]));
                            r_plane = critical_plane_above(true_delinearized(right[axis]));
                        } else {
                            l_plane = critical_plane_above(true_delinearized(left[axis]));
                            r_plane = critical_plane_below(true_delinearized(right[axis]));
                        }
                        for (int i = 0; i < 8; i++) {
                            if (std::abs(r_plane - l_plane) <= 1) {
                                break;
                            }
                            int m_plane = (l_plane + r_plane) / 2;
                            // Clamp to valid CRITICAL_PLANES range [0, 254]
                            m_plane = std::clamp(m_plane, 0, 254);
                            const double midPlaneCoordinate = CRITICAL_PLANES[m_plane];
                            auto mid = set_coordinate(left, midPlaneCoordinate, right, axis);
                            const double midHue = hue_of(mid);
                            if (are_in_cyclic_order(left_hue, target_hue, midHue)) {
                                right = mid;
                                r_plane = m_plane;
                            } else {
                                left = mid;
                                left_hue = midHue;
                                l_plane = m_plane;
                            }
                        }
                    }
                }
                return midpoint(left, right);
            }

            // Newton Iteration (core HCT solver)
            // Given hue (radians), chroma, and target Y, iteratively find J (CAM16
            // lightness) such that the forward transform (J, C, h) -> RGB -> Y
            // produces a Y value matching the target.
            //
            // Uses Newton's method with Jacobian approximation fn'(J) ~ 2*fn(J)/J.
            // Maximum 5 iterations, early exit if |fn(J) - Y| < 0.002.
            static auto find_result_by_j(const double hue_radians, const double chroma, const double y) -> std::int32_t {
                // Initial J estimate: approximate mapping from Y to CAM16 lightness
                double j = std::sqrt(y) * 11.0;

                const auto& vc = ViewingConditions::DEFAULT;

                // Pre-compute iteration-invariant terms
                const double tInnerCoeff = 1.0 / std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
                const double eHue = 0.25 * (std::cos(hue_radians + 2.0) + 3.8);
                const double p1 = eHue * (50000.0 / 13.0) * vc.nc * vc.ncb;
                const double hSin = std::sin(hue_radians);
                const double hCos = std::cos(hue_radians);

                for (int iteration_round = 0; iteration_round < NUM_ITERATIONS; iteration_round++) {
                    const double jNormalized = j / 100.0;

                    // alpha = C / sqrt(J/100), with guard for degenerate cases
                    const double alpha = chroma == 0.0 || j == 0.0 ? 0.0 : chroma / std::sqrt(jNormalized);

                    // t (CAM16 eccentricity factor) from chroma
                    const double t = std::pow(alpha * tInnerCoeff, 1.0 / 0.9);

                    // Achromatic response ac = aw * (J/100)^(1/(c*z))
                    const double ac = vc.aw * std::pow(jNormalized, 1.0 / vc.c / vc.z);
                    const double p2 = ac / vc.nbb;

                    // gamma relates achromatic/chromatic responses to opponent colors
                    const double gamma = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);

                    // Opponent color coordinates (a: red-green, b: yellow-blue)
                    const double a = gamma * hCos;
                    const double b = gamma * hSin;

                    // Inverse CAM16: opponent colors -> adapted cone responses
                    const double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
                    const double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
                    const double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;

                    // Inverse chromatic adaptation: adapted -> linear scaled cone responses
                    const double rCScaled = inverse_chromatic_adaptation(rA);
                    const double gCScaled = inverse_chromatic_adaptation(gA);
                    const double bCScaled = inverse_chromatic_adaptation(bA);

                    // Recover linear RGB from scaled discount space
                    const std::array scaledRow = {rCScaled, gCScaled, bCScaled};
                    auto linrgb = matrix_multiply(scaledRow, LINRGB_FROM_SCALED_DISCOUNT);

                    // Check for out-of-gamut result (negative means gamut boundary exceeded)
                    if (linrgb[0] < 0 || linrgb[1] < 0 || linrgb[2] < 0) {
                        return 0;
                    }

                    // Compute reconstructed Y from linear RGB
                    constexpr double kR = Y_FROM_LINRGB[0];
                    constexpr double kG = Y_FROM_LINRGB[1];
                    constexpr double kB = Y_FROM_LINRGB[2];
                    const double fnj = kR * linrgb[0] + kG * linrgb[1] + kB * linrgb[2];

                    if (fnj <= 0) {
                        return 0;
                    }

                    // Check convergence or final iteration
                    if (iteration_round == 4 || std::abs(fnj - y) < 0.002) {
                        // Final validation: if any channel exceeds sRGB range, return 0
                        if (linrgb[0] > 100.01 || linrgb[1] > 100.01 || linrgb[2] > 100.01) {
                            return 0;
                        }
                        return argb_from_linrgb100(linrgb[0], linrgb[1], linrgb[2]);
                    }

                    // Newton step: J_{n+1} = J_n - (fn(J_n) - Y) * J_n / (2 * fn(J_n))
                    // Uses the approximation fn'(J) ~ 2*fn(J)/J
                    j = j - (fnj - y) * j / (2.0 * fnj);
                }
                return 0;
            }
        };

        // HCT (Hue-Chroma-Tone) Color Space Wrapper
        // Thin wrapper over CAM16 for convenient HCT color representation.
        class Hct {
            double hue_ = 0, chroma_ = 0, tone_ = 0;
            std::int32_t argb_ = 0;
        public:
            Hct() = default;

            static auto from_int(const std::int32_t argb) -> Hct {
                const auto cam = Cam16::from_argb(argb);
                return Hct{cam.hue, cam.chroma, lstar_from_argb(argb), argb};
            }

            [[nodiscard]] auto to_int() const -> std::int32_t {
                return argb_;
            }

            [[nodiscard]] auto hue() const -> double {
                return hue_;
            }

            [[nodiscard]] auto chroma() const -> double {
                return chroma_;
            }

            [[nodiscard]] auto tone() const -> double {
                return tone_;
            }
        private:
            Hct(const double h, const double c, const double t, const std::int32_t a) : hue_(h),
                                                                                        chroma_(c),
                                                                                        tone_(t),
                                                                                        argb_(a) {}
        };

        // TonalPalette
        // Maps a (hue, chroma) pair to a set of colors at varying tones (0-100).
        // Ported from material_color_utilities palette_ts.dart.
        class TonalPalette {
            double hue_ = 0, chroma_ = 0;
        public:
            TonalPalette() = default;

            static auto from_hue_and_chroma(const double hue, const double chroma) -> TonalPalette {
                return TonalPalette{hue, chroma};
            }

            [[nodiscard]] auto tone(const double t) const -> type::Color {
                const double adjustedHue = hue_;
                // Yellow T99 fix: yellow hues (70-120°) at T99 use average of T98+T100
                if (std::abs(t - 99.0) < 0.5 && adjustedHue >= 70.0 && adjustedHue <= 120.0) {
                    const auto c98 = HctSolver::solve_to_int(adjustedHue, chroma_, 98.0);
                    const auto c100 = HctSolver::solve_to_int(adjustedHue, chroma_, 100.0);
                    const auto avg = argb_from_rgb((red_from_argb(c98) + red_from_argb(c100)) / 2,
                                                   (green_from_argb(c98) + green_from_argb(c100)) / 2,
                                                   (blue_from_argb(c98) + blue_from_argb(c100)) / 2);
                    return to_color(avg);
                }
                const auto argb = HctSolver::solve_to_int(adjustedHue, chroma_, t);
                return to_color(argb);
            }

            [[nodiscard]] auto hue() const -> double {
                return hue_;
            }

            [[nodiscard]] auto chroma() const -> double {
                return chroma_;
            }
        private:
            TonalPalette(const double h, const double c) : hue_(h),
                                                           chroma_(c) {}

            static auto to_color(const std::int32_t argb) -> type::Color {
                return argb_to_color(argb);
            }
        };

        // DynamicScheme (tonalSpot)
        // Holds the 6 tonal palettes for a tonalSpot color scheme variant
        // alongside isDark / contrastLevel flags.
        class DynamicScheme {
        public:
            Hct source_hct_;
            TonalPalette primary_palette_, secondary_palette_, tertiary_palette_;
            TonalPalette neutral_palette_, neutral_variant_palette_, error_palette_;
            bool is_dark_;
            double contrast_level_;

            DynamicScheme(const Hct& source, const bool isDark, const double contrast_level) : source_hct_(source),
                                                                                               is_dark_(isDark),
                                                                                               contrast_level_(contrast_level) {
                const auto h = source.hue();
                primary_palette_ = TonalPalette::from_hue_and_chroma(h, 36.0);
                secondary_palette_ = TonalPalette::from_hue_and_chroma(h, 16.0);
                tertiary_palette_ = TonalPalette::from_hue_and_chroma(sanitize_degrees(h + 60.0), 24.0);
                neutral_palette_ = TonalPalette::from_hue_and_chroma(h, 6.0);
                neutral_variant_palette_ = TonalPalette::from_hue_and_chroma(h, 8.0);
                error_palette_ = TonalPalette::from_hue_and_chroma(25.0, 84.0);
            }
        };

        // ContrastCurve
        // Interpolates contrast level values at standard breakpoints:
        //   cl = -1.0 -> low
        //   cl =  0.0 -> normal
        //   cl =  1.0 -> high
        //   cl =  2.0 -> highest
        struct ContrastCurve {
            double low, normal, high, highest;

            [[nodiscard]] auto get(const double cl) const -> double {
                if (cl <= -1.0) {
                    return low;
                }
                if (cl < 0.0) {
                    return low + (normal - low) * (cl + 1.0);
                }
                if (cl < 1.0) {
                    return normal + (high - normal) * cl;
                }
                return high + (highest - high) * (cl - 1.0);
            }
        };

        class DynamicColor; // forward declaration for ToneDeltaPair

        // Polarity for ToneDeltaPair
        enum class Polarity {
            Nearer,
            Lighter,
            Darker,
            NoPreference
        };

        // ToneDeltaPair
        struct ToneDeltaPair {
            const DynamicColor* role_a;
            const DynamicColor* role_b;
            double delta;
            Polarity polarity;
            bool stay_together;
        };

        // DynamicColor
        // Core color role resolver: given a DynamicScheme, resolves a "color role"
        // (e.g. primary, onPrimary) to an actual ARGB color via a palette + tone
        // pipeline with optional light-foreground and tone-delta-pair adjustments.
        class DynamicColor {
        public:
            using PaletteFn = const TonalPalette& (*)(const DynamicScheme&);
            using ToneFn = double (*)(const DynamicScheme&);
            using BgFn = const DynamicColor& (*)(const DynamicScheme&);
        private:
            PaletteFn paletteFn_ = nullptr;
            ToneFn toneFn_ = nullptr;
            BgFn bgFn_ = nullptr;
            ContrastCurve curve_{};
            std::optional<ToneDeltaPair> pair_;
            bool enableLightFg_ = true;
        public:
            DynamicColor() = default; // placeholder for noBg

            DynamicColor(const PaletteFn p, const ToneFn t, const BgFn b, const ContrastCurve& cc, const std::optional<ToneDeltaPair>& pr = std::nullopt, const bool elf = true) : paletteFn_(p),
                toneFn_(t),
                bgFn_(b),
                curve_(cc),
                pair_(pr),
                enableLightFg_(elf) {}

            [[nodiscard]] auto get_argb(const DynamicScheme& s) const -> std::int32_t;
            [[nodiscard]] auto get_tone(const DynamicScheme& s) const -> double;

            [[nodiscard]] auto get_palette(const DynamicScheme& s) const -> const TonalPalette& {
                return paletteFn_(s);
            }
        };

        // WCAG contrast ratio
        inline auto ratio_of_tones(const double ta, const double tb) -> double {
            const auto ya = y_from_lstar(ta);
            const auto yb = y_from_lstar(tb);
            const auto l = std::max(ya, yb);
            const auto d = std::min(ya, yb);
            return (l + 0.05) / (d + 0.05);
        }

        // Find foreground tone meeting target contrast ratio against background
        inline auto foreground_tone(const double bg_tone, const double target_ratio) -> double {
            if (ratio_of_tones(100.0, bg_tone) >= target_ratio) {
                return 100.0;
            }
            if (ratio_of_tones(0.0, bg_tone) >= target_ratio) {
                return 0.0;
            }
            // Pick whichever is closer (MCU prefers light foreground when possible)
            return ratio_of_tones(100.0, bg_tone) >= ratio_of_tones(0.0, bg_tone) ? 100.0 : 0.0;
        }

        // noBg: placeholder for roles with enableLightForeground=false
        // Returns a const reference to a default-constructed DynamicColor.
        // When used as a BgFn, getTone() will never be called on it because
        // enableLightFg_ is false for the foreground role.
        inline auto no_bg(const DynamicScheme& /*unused*/) -> const DynamicColor& {
            static DynamicColor dummy;
            return dummy;
        }

        // ARGB resolution
        inline auto DynamicColor::get_argb(const DynamicScheme& s) const -> Argb {
            const auto color = get_palette(s).tone(get_tone(s));
            return argb_from_rgb(static_cast<int>(std::round(color.r * 255.0)), static_cast<int>(std::round(color.g * 255.0)), static_cast<int>(std::round(color.b * 255.0)));
        }

        // Full tone resolution pipeline
        inline auto DynamicColor::get_tone(const DynamicScheme& s) const -> double {
            auto tone = toneFn_(s);
            if (enableLightFg_) {
                const auto bg = bgFn_(s).get_tone(s);
                const auto target = curve_.get(s.contrast_level_);
                tone = foreground_tone(bg, target);
            }
            if (pair_.has_value()) {
                const auto& p = pair_.value();
                const auto tB = p.role_b->get_tone(s);
                switch (p.polarity) {
                    case Polarity::Nearer:
                        if (std::abs(tone - tB) < p.delta) {
                            tone = tone < tB ? tB - p.delta : tB + p.delta;
                        }
                        break;
                    case Polarity::Lighter:
                        tone = std::max(tone, tB + p.delta);
                        break;
                    case Polarity::Darker:
                        tone = std::min(tone, tB - p.delta);
                        break;
                    case Polarity::NoPreference:
                        if (std::abs(tone - tB) < p.delta) {
                            const auto dLight = tone + p.delta - tB;
                            const auto dDark = tB - (tone - p.delta);
                            tone = dLight < dDark ? tB - p.delta : tB + p.delta;
                        }
                        break;
                }
                tone = std::clamp(tone, 0.0, 100.0);
            }
            return tone;
        }

        // MaterialDynamicColors
        // All color role definitions for `tonalSpot` scheme variant.
        // Uses `contrastLevel=0.0` as base; `getTone` applies contrast curves dynamically.
        struct MaterialDynamicColors {
            // Palette accessors
            static auto pri_pal(const DynamicScheme& s) -> const TonalPalette& {
                return s.primary_palette_;
            }

            static auto sec_pal(const DynamicScheme& s) -> const TonalPalette& {
                return s.secondary_palette_;
            }

            static auto ter_pal(const DynamicScheme& s) -> const TonalPalette& {
                return s.tertiary_palette_;
            }

            static auto neu_pal(const DynamicScheme& s) -> const TonalPalette& {
                return s.neutral_palette_;
            }

            static auto nvp_pal(const DynamicScheme& s) -> const TonalPalette& {
                return s.neutral_variant_palette_;
            }

            static auto err_pal(const DynamicScheme& s) -> const TonalPalette& {
                return s.error_palette_;
            }

            // --- Tone infrastructure ---
            struct TonePair {
                double dark;
                double light;
            };

            enum Tone : int {
                Tone40_80,
                Tone100_20,
                Tone90_30,
                Tone10_90,
                Tone90_90,
                Tone80_80,
                Tone80_40,
                Tone10_10,
                Tone30_30,
                Tone30_80,
                Tone98_6,
                Tone87_6,
                Tone98_24,
                Tone100_4,
                Tone96_10,
                Tone94_12,
                Tone92_22,
                Tone90_24,
                Tone50_60,
                Tone80_30,
                Tone20_90,
                Tone95_20,
                Tone0_0,
                ToneCount
            };

            static constexpr std::array<TonePair, ToneCount> kTones{
                {
                    {.dark = 80, .light = 40},
                    {.dark = 20, .light = 100},
                    {.dark = 30, .light = 90},
                    {.dark = 90, .light = 10},
                    {.dark = 90, .light = 90},
                    {.dark = 80, .light = 80},
                    {.dark = 40, .light = 80},
                    {.dark = 10, .light = 10},
                    {.dark = 30, .light = 30},
                    {.dark = 80, .light = 30},
                    {.dark = 6, .light = 98},
                    {.dark = 6, .light = 87},
                    {.dark = 24, .light = 98},
                    {.dark = 4, .light = 100},
                    {.dark = 10, .light = 96},
                    {.dark = 12, .light = 94},
                    {.dark = 22, .light = 92},
                    {.dark = 24, .light = 90},
                    {.dark = 60, .light = 50},
                    {.dark = 30, .light = 80},
                    {.dark = 90, .light = 20},
                    {.dark = 20, .light = 95},
                    {.dark = 0, .light = 0},
                }
            };

            static auto tone(const Tone idx, const DynamicScheme& s) -> double {
                const auto& t = kTones[idx];
                return s.is_dark_ ? t.dark : t.light;
            }

            // --- Role definitions (compact, using captureless lambda for ToneFn) ---

            // Background: highest surface
            static auto highest_surface(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_24, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            // Primary group
            static auto primary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone40_80, s);
                    },
                    highest_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 7}
                };
                return k;
            }

            static auto on_primary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone100_20, s);
                    },
                    primary,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto primary_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_30, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_primary_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_90, s);
                    },
                    primary_container,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto primary_fixed(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_90, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto primary_fixed_dim(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone80_80, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_primary_fixed(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_10, s);
                    },
                    primary_fixed_dim,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto on_primary_fixed_variant(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone30_30, s);
                    },
                    primary_fixed_dim,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 11}
                };
                return k;
            }

            // Secondary group
            static auto secondary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone40_80, s);
                    },
                    highest_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 7}
                };
                return k;
            }

            static auto on_secondary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone100_20, s);
                    },
                    secondary,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto secondary_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_30, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_secondary_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_90, s);
                    },
                    secondary_container,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto secondary_fixed(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_90, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto secondary_fixed_dim(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone80_80, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_secondary_fixed(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_10, s);
                    },
                    secondary_fixed_dim,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto on_secondary_fixed_variant(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    sec_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone30_30, s);
                    },
                    secondary_fixed_dim,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 11}
                };
                return k;
            }

            // Tertiary group
            static auto tertiary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone40_80, s);
                    },
                    highest_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 7}
                };
                return k;
            }

            static auto on_tertiary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone100_20, s);
                    },
                    tertiary,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto tertiary_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_30, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_tertiary_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_90, s);
                    },
                    tertiary_container,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto tertiary_fixed(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_90, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto tertiary_fixed_dim(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone80_80, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_tertiary_fixed(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_10, s);
                    },
                    tertiary_fixed_dim,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto on_tertiary_fixed_variant(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    ter_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone30_30, s);
                    },
                    tertiary_fixed_dim,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 11}
                };
                return k;
            }

            // Error group
            static auto error(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    err_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone40_80, s);
                    },
                    highest_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 7}
                };
                return k;
            }

            static auto on_error(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    err_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone100_20, s);
                    },
                    error,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto error_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    err_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_30, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            static auto on_error_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    err_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_90, s);
                    },
                    error_container,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            // Surface group (all enableLightForeground=false — they ARE surfaces)
            static auto surface(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone98_6, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_dim(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone87_6, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_bright(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone98_24, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_container_lowest(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone100_4, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_container_low(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone96_10, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_container(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone94_12, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_container_high(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone92_22, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto surface_container_highest(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_24, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            // On-surface
            static auto on_surface(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone10_90, s);
                    },
                    highest_surface,
                    {.low = 4.5, .normal = 7, .high = 11, .highest = 21}
                };
                return k;
            }

            static auto surface_variant(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    nvp_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone90_30, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto on_surface_variant(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    nvp_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone30_80, s);
                    },
                    highest_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 11}
                };
                return k;
            }

            // Outline
            static auto outline(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    nvp_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone50_60, s);
                    },
                    highest_surface,
                    {.low = 1.5, .normal = 3, .high = 4.5, .highest = 7}
                };
                return k;
            }

            static auto outline_variant(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    nvp_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone80_30, s);
                    },
                    highest_surface,
                    {.low = 1, .normal = 1, .high = 3, .highest = 4.5}
                };
                return k;
            }

            // Inverse
            static auto inverse_surface(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone20_90, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto inverse_on_surface(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone95_20, s);
                    },
                    inverse_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 11}
                };
                return k;
            }

            static auto inverse_primary(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    pri_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone80_40, s);
                    },
                    inverse_surface,
                    {.low = 3, .normal = 4.5, .high = 7, .highest = 7}
                };
                return k;
            }

            // Other
            static auto shadow(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone0_0, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }

            static auto scrim(const DynamicScheme& /*unused*/) -> const DynamicColor& {
                static DynamicColor k{
                    neu_pal,
                    [](const DynamicScheme& s) -> double {
                        return tone(Tone0_0, s);
                    },
                    no_bg,
                    {.low = 0, .normal = 0, .high = 0, .highest = 0},
                    std::nullopt,
                    false
                };
                return k;
            }
        };
    } // namespace detail

    struct ColorScheme {
        type::Color primary;
        type::Color on_primary;
        type::Color primary_container;
        type::Color on_primary_container;
        type::Color primary_fixed;
        type::Color primary_fixed_dim;
        type::Color on_primary_fixed;
        type::Color on_primary_fixed_variant;
        type::Color secondary;
        type::Color on_secondary;
        type::Color secondary_container;
        type::Color on_secondary_container;
        type::Color secondary_fixed;
        type::Color secondary_fixed_dim;
        type::Color on_secondary_fixed;
        type::Color on_secondary_fixed_variant;
        type::Color tertiary;
        type::Color on_tertiary;
        type::Color tertiary_container;
        type::Color on_tertiary_container;
        type::Color tertiary_fixed;
        type::Color tertiary_fixed_dim;
        type::Color on_tertiary_fixed;
        type::Color on_tertiary_fixed_variant;
        type::Color error;
        type::Color on_error;
        type::Color error_container;
        type::Color on_error_container;
        type::Color surface;
        type::Color surface_dim;
        type::Color surface_bright;
        type::Color surface_container_lowest;
        type::Color surface_container_low;
        type::Color surface_container;
        type::Color surface_container_high;
        type::Color surface_container_highest;
        type::Color on_surface;
        type::Color surface_variant;
        type::Color on_surface_variant;
        type::Color outline;
        type::Color outline_variant;
        type::Color inverse_surface;
        type::Color inverse_on_surface;
        type::Color inverse_primary;
        type::Color shadow;
        type::Color scrim;

        static auto from_seed(const type::Color seed_color, const bool isDark = false, const float contrast_level = 0.0F) -> ColorScheme {
            const auto argb = detail::argb_from_rgb(static_cast<int>(std::round(seed_color.r * 255.0)),
                                                    static_cast<int>(std::round(seed_color.g * 255.0)),
                                                    static_cast<int>(std::round(seed_color.b * 255.0)));
            const auto hct = detail::Hct::from_int(argb);
            const detail::DynamicScheme scheme(hct, isDark, contrast_level);

            // DEBUG: force surface to verify HCT solver
            const auto test_args = detail::HctSolver::solve_to_int(120.0, 6.0, 98.0);
            const auto test_color = detail::argb_to_color(test_args);
            __debugbreak(); // set breakpoint here, check test_args and test_color

            auto resolve = [&](const detail::DynamicColor& role) -> type::Color {
                return detail::argb_to_color(role.get_argb(scheme));
            };

            return {
                .primary = resolve(detail::MaterialDynamicColors::primary(scheme)),
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
                .surface_variant = resolve(detail::MaterialDynamicColors::surface_variant(scheme)),
                .on_surface_variant = resolve(detail::MaterialDynamicColors::on_surface_variant(scheme)),
                .outline = resolve(detail::MaterialDynamicColors::outline(scheme)),
                .outline_variant = resolve(detail::MaterialDynamicColors::outline_variant(scheme)),
                .inverse_surface = resolve(detail::MaterialDynamicColors::inverse_surface(scheme)),
                .inverse_on_surface = resolve(detail::MaterialDynamicColors::inverse_on_surface(scheme)),
                .inverse_primary = resolve(detail::MaterialDynamicColors::inverse_primary(scheme)),
                .shadow = resolve(detail::MaterialDynamicColors::shadow(scheme)),
                .scrim = resolve(detail::MaterialDynamicColors::scrim(scheme)),
            };
        }
    };
} // namespace neko::color
