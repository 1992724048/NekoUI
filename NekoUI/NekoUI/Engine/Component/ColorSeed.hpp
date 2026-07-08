#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <glm/glm.hpp>
#include "../../Type.hpp"

namespace neko::seed {
    struct ColorScheme; // forward decl

    namespace detail {
        // === ARGB Utility ===
        constexpr auto argbFromRgb(int r, int g, int b) -> std::int32_t {
            return (255 << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
        }

        constexpr auto alphaFromArgb(std::int32_t a) -> int {
            return (a >> 24) & 0xFF;
        }

        constexpr auto redFromArgb(std::int32_t a) -> int {
            return (a >> 16) & 0xFF;
        }

        constexpr auto greenFromArgb(std::int32_t a) -> int {
            return (a >> 8) & 0xFF;
        }

        constexpr auto blueFromArgb(std::int32_t a) -> int {
            return a & 0xFF;
        }

        // === sRGB Gamma ===
        inline auto linearized(int rgbComponent) -> double {
            double n = rgbComponent / 255.0;
            return (n <= 0.040449936) ? (n / 12.92) : std::pow((n + 0.055) / 1.055, 2.4);
        }

        inline auto delinearized(double rgbComponent) -> int {
            double n;
            if (rgbComponent <= 0.0031308) {
                n = rgbComponent * 12.92;
            } else {
                n = 1.055 * std::pow(rgbComponent, 1.0 / 2.4) - 0.055;
            }
            return static_cast<int>(std::round(n * 255.0));
        }

        inline auto argbFromLinrgb(double r, double g, double b) -> std::int32_t {
            return argbFromRgb(delinearized(r), delinearized(g), delinearized(b));
        }

        // === L* ↔ Y (CIE Lightness ↔ Relative Luminance) ===
        inline auto yFromLstar(double lstar) -> double {
            return (lstar > 8.0) ? std::pow((lstar + 16.0) / 116.0, 3.0) : lstar / 903.3;
        }

        inline auto lstarFromY(double y) -> double {
            return (y > 0.008856) ? 116.0 * std::cbrt(y) - 16.0 : y * 903.3;
        }

        inline auto lstarFromArgb(std::int32_t argb) -> double {
            auto y = 0.2126 * linearized(redFromArgb(argb)) + 0.7152 * linearized(greenFromArgb(argb)) + 0.0722 * linearized(blueFromArgb(argb));
            return lstarFromY(y);
        }

        inline auto argbFromLstar(double lstar) -> std::int32_t {
            auto c = delinearized(yFromLstar(lstar));
            return argbFromRgb(c, c, c);
        }

        // === Math Helpers ===
        inline auto sanitizeDegrees(double d) -> double {
            d = std::fmod(d, 360.0);
            return (d < 0) ? d + 360.0 : d;
        }

        // Signum function: -1 for negative, 0 for zero, 1 for positive
        inline auto signum(double x) -> double {
            return (x < 0.0) ? -1.0 : ((x > 0.0) ? 1.0 : 0.0);
        }

        // === ViewingConditions (CAM16 viewing conditions for D65) ===
        // Ported from material_color_utilities viewing_conditions.ts / viewing_conditions.dart
        struct ViewingConditions {
            double n = 0.0, aw = 0.0, nbb = 0.0, ncb = 0.0;
            double c = 0.0, nc = 0.0, fl = 0.0, fLRoot = 0.0, z = 0.0;

            static auto make() -> ViewingConditions {
                // Default D65 white point (CIE standard illuminant D65)
                double whitePoint[] = {0.95047, 1.0, 1.08883};

                // adaptingLuminance: standard sRGB-like conditions
                // Default = (200/π) * Y_from_L*(50) / 100
                double adaptingLuminance = (200.0 / 3.141592653589793) * yFromLstar(50.0) / 100.0;

                double backgroundLstar = 50.0;
                double surround = 2.0; // average surround

                // Transform white point XYZ to CAT02 cone responses
                double rW = whitePoint[0] * 0.401288 + whitePoint[1] * 0.650173 + whitePoint[2] * -0.051461;
                double gW = whitePoint[0] * -0.250268 + whitePoint[1] * 1.204414 + whitePoint[2] * 0.045854;
                double bW = whitePoint[0] * -0.002079 + whitePoint[1] * 0.048952 + whitePoint[2] * 0.953127;

                // Surround → CAM16 surround factor f (0.8–1.0)
                double f = 0.8 + (surround / 10.0);

                // CAM16 exponential non-linearity c
                double c;
                if (f >= 0.9) {
                    c = 0.59 + (0.69 - 0.59) * ((f - 0.9) * 10.0);
                } else {
                    c = 0.525 + (0.59 - 0.525) * ((f - 0.8) * 10.0);
                }

                // Degree of adaptation d
                double d = f * (1.0 - (1.0 / 3.6) * std::exp((-adaptingLuminance - 42.0) / 92.0));
                d = std::clamp(d, 0.0, 1.0);

                // Chromatic induction factor nc = f (surround factor)
                double nc = f;

                // Discounted cone responses to white point
                // Note: uses 100.0 (luminance of reference white), NOT whitePoint[1].
                double rgbD[3] = {d * (100.0 / rW) + 1.0 - d, d * (100.0 / gW) + 1.0 - d, d * (100.0 / bW) + 1.0 - d,};

                // k factor for luminance-level adaptation
                double k = 1.0 / (5.0 * adaptingLuminance + 1.0);
                double k4 = k * k * k * k;
                double k4F = 1.0 - k4;

                // Luminance-level adaptation factor fl
                double fl = (k4 * 5.0 * adaptingLuminance) + 0.1 * k4F * k4F * std::pow(5.0 * adaptingLuminance, 1.0 / 3.0);
                double fLRoot = std::pow(fl, 0.25);

                // n: ratio of background relative luminance to white relative luminance
                double n = yFromLstar(backgroundLstar) / whitePoint[1];

                // Base exponential nonlinearity z
                double z = 1.48 + std::sqrt(n);

                // Luminance-level induction factors
                double nbb = 0.725 / std::pow(n, 0.2);
                double ncb = nbb;

                // Non-linear cone responses for white point (with fl adaptation)
                double rgbAFactors[3] = {std::pow(fl * rgbD[0] * rW / 100.0, 0.42), std::pow(fl * rgbD[1] * gW / 100.0, 0.42), std::pow(fl * rgbD[2] * bW / 100.0, 0.42),};

                double rgbA[3] = {(400.0 * rgbAFactors[0]) / (rgbAFactors[0] + 27.13), (400.0 * rgbAFactors[1]) / (rgbAFactors[1] + 27.13), (400.0 * rgbAFactors[2]) / (rgbAFactors[2] + 27.13),};

                // Achromatic response for white
                double aw = ((40.0 * rgbA[0] + 20.0 * rgbA[1] + rgbA[2]) / 20.0) * nbb;

                return {n, aw, nbb, ncb, c, nc, fl, fLRoot, z};
            }

            static ViewingConditions DEFAULT;
        };

        inline ViewingConditions ViewingConditions::DEFAULT = make();

        // === CAM16 forward transform ===
        struct Cam16 {
            double hue = 0.0, chroma = 0.0, j = 0.0, q = 0.0, m = 0.0, s = 0.0, jstar = 0.0;

            // Chromatic adaptation: non-linear compression of cone responses
            // Uses the CAM16 0.42 power + 400/(x+27.13) sigmoid
            static auto chromaticAdaptation(double component) -> double {
                double af = std::pow(std::abs(component), 0.42);
                return signum(component) * 400.0 * af / (af + 27.13);
            }

            static auto fromArgb(std::int32_t argb) -> Cam16 {
                auto rL = linearized(redFromArgb(argb));
                auto gL = linearized(greenFromArgb(argb));
                auto bL = linearized(blueFromArgb(argb));

                auto x = 0.41233895 * rL + 0.35762064 * gL + 0.18051042 * bL;
                auto y = 0.2126 * rL + 0.7152 * gL + 0.0722 * bL;
                auto zV = 0.01932141 * rL + 0.11916382 * gL + 0.95034478 * bL;

                auto& vc = ViewingConditions::DEFAULT;
                double rC = x * 0.401288 + y * 0.650173 + zV * -0.051461;
                double gC = x * -0.250268 + y * 1.204414 + zV * 0.045854;
                double bC = x * -0.002079 + y * 0.048952 + zV * 0.953127;

                // Apply cone response adaptation (D65 adapting white → identity)
                double rD = rC;
                double gD = gC;
                double bD = bC;

                // Non-linear compression via CAM16 chromatic adaptation
                double rA = chromaticAdaptation(rD / 100.0);
                double gA = chromaticAdaptation(gD / 100.0);
                double bA = chromaticAdaptation(bD / 100.0);

                // Achromatic response a
                double a = vc.nbb * (2.0 * rA + gA + 0.05 * bA);
                double j = 100.0 * std::pow(a / vc.aw, vc.c * vc.z);

                // Hue
                double hRad = std::atan2(bA - gA, rA - gA);
                double hDeg = hRad * 180.0 / std::numbers::pi;
                if (hDeg < 0) {
                    hDeg += 360.0;
                }

                double hRadians = hDeg * std::numbers::pi / 180.0;
                double eT = 0.25 * (std::cos(hRadians + 2.0) + 3.8);

                // Chroma
                double tBase = 50000.0 / 13.0 * vc.nc * vc.ncb * eT;
                double tNum = std::sqrt(rA * rA + gA * gA - 2.0 * rA * gA - rA * bA + gA * bA);
                double tDen = rA + gA + 1.05 * bA + 0.305;
                double t = tBase * tNum / tDen;

                double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
                double chroma = alpha * std::sqrt(j / 100.0);

                return {hDeg, chroma, j, 0.0, 0.0, 0.0, 0.0};
            }
        };

        // === HCT Solver (Newton Iteration) ===
        // Ported from material_color_utilities hct_solver.dart / hct_solver.ts
        // See: https://github.com/material-foundation/material-color-utilities
        struct HctSolver {
            static constexpr int NUM_ITERATIONS = 5;

            // === Matrix Constants ===
            // SCALED_DISCOUNT_FROM_LINRGB: Maps linear RGB to scaled discount space.
            // Embeds the CAT02 chromatic adaptation transform with D65 illuminant
            // discounting baked in, allowing hueOf() to work directly on linear RGB.
            static constexpr double SCALED_DISCOUNT_FROM_LINRGB[3][3] = {
                {0.001200833568784504, 0.002389694492170889, 0.0002795742885861124},
                {0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398},
                {0.00010146692491640572, 0.0005364214359186694, 0.0032979401770712076},
            };

            // LINRGB_FROM_SCALED_DISCOUNT: Inverse of SCALED_DISCOUNT_FROM_LINRGB.
            // Used in findResultByJ to recover linear RGB from adapted cone responses.
            static constexpr double LINRGB_FROM_SCALED_DISCOUNT[3][3] = {
                {1373.2198709594231, -1100.4251190754821, -7.278681089101213},
                {-271.815969077903, 559.6580465940733, -32.46047482791194},
                {1.9622899599665666, -57.173814538844006, 308.7233197812385},
            };

            // Y_FROM_LINRGB: Coefficients for computing Y (relative luminance) from
            // linear RGB: Y = 0.2126*R + 0.7152*G + 0.0722*B
            static constexpr double Y_FROM_LINRGB[3] = {0.2126, 0.7152, 0.0722};

            // CRITICAL_PLANES: Pre-computed sRGB transfer function values at critical
            // points used by bisectToLimit for binary search on the gamut boundary.
            // 255 entries correspond to the midpoints between [0, 255] quantization levels.
            static constexpr double CRITICAL_PLANES[255] = {
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

            // === Public Entry Points ===

            // Solve for an ARGB color given hue (degrees), chroma, and L* (tone).
            // If the desired chroma is unreachable at this hue+tone, returns the
            // closest reachable color on the sRGB gamut boundary.
            static auto solveToInt(double hueDeg, double chroma, double lstar) -> std::int32_t {
                if (chroma < 0.0001 || lstar < 0.0001 || lstar > 99.9999) {
                    return argbFromLstar(lstar);
                }
                hueDeg = sanitizeDegrees(hueDeg);
                double hueRadians = hueDeg / 180.0 * 3.141592653589793;
                double y = yFromLstar(lstar);
                std::int32_t exactAnswer = findResultByJ(hueRadians, chroma, y);
                if (exactAnswer != 0) {
                    return exactAnswer;
                }
                // Chroma was unreachable, fall back to the gamut boundary at this hue+Y
                std::array<double, 3> linrgb = bisectToLimit(y, hueRadians);
                return argbFromLinrgb100(linrgb[0], linrgb[1], linrgb[2]);
            }

            // Solve for a Cam16 color. Convenience wrapper.
            static auto solveToCam(double hueDeg, double chroma, double lstar) -> Cam16 {
                return Cam16::fromArgb(solveToInt(hueDeg, chroma, lstar));
            }
        private:
            // === Private Helpers ===

            static auto sanitizeRadians(double angle) -> double {
                return std::fmod(angle + 8.0 * 3.141592653589793, 2.0 * 3.141592653589793);
            }

            // Signum function: -1 for negative, 0 for zero, 1 for positive
            static auto signum(double x) -> double {
                return (x < 0.0) ? -1.0 : ((x > 0.0) ? 1.0 : 0.0);
            }

            // Matrix multiply: 1x3 row vector x 3x3 matrix -> 3-element vector
            static auto matrixMultiply(const double row[3], const double matrix[3][3]) -> std::array<double, 3> {
                return std::array<double, 3>{
                    row[0] * matrix[0][0] + row[1] * matrix[0][1] + row[2] * matrix[0][2],
                    row[0] * matrix[1][0] + row[1] * matrix[1][1] + row[2] * matrix[1][2],
                    row[0] * matrix[2][0] + row[1] * matrix[2][1] + row[2] * matrix[2][2],
                };
            }

            // === Forward/Inverse Chromatic Adaptation ===
            // These implement the nonlinear chromatic adaptation from CAM16.
            // Forward: linear cone response -> adapted response (using 0.42 power)
            // Inverse: adapted response -> linear cone response

            static auto chromaticAdaptation(double component) -> double {
                double af = std::pow(std::abs(component), 0.42);
                return signum(component) * 400.0 * af / (af + 27.13);
            }

            static auto inverseChromaticAdaptation(double adapted) -> double {
                double adaptedAbs = std::abs(adapted);
                double base = std::max(0.0, 27.13 * adaptedAbs / (400.0 - adaptedAbs));
                return signum(adapted) * std::pow(base, 1.0 / 0.42);
            }

            // === sRGB Transfer Helpers (for [0, 100] linear RGB range) ===

            // Delinearize: [0, 100] linear -> [0, 255] float (no clamping).
            // Used only in bisectToLimit for critical plane computation.
            static auto trueDelinearized(double rgbComponent) -> double {
                double normalized = rgbComponent / 100.0;
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
            static auto delinearizedInt(double rgbComponent) -> int {
                double normalized = rgbComponent / 100.0;
                double d;
                if (normalized <= 0.0031308) {
                    d = normalized * 12.92;
                } else {
                    d = 1.055 * std::pow(normalized, 1.0 / 2.4) - 0.055;
                }
                return std::clamp(static_cast<int>(std::round(d * 255.0)), 0, 255);
            }

            // ARGB from [0, 100] linear RGB values (MCU convention).
            static auto argbFromLinrgb100(double r, double g, double b) -> std::int32_t {
                return argbFromRgb(delinearizedInt(r), delinearizedInt(g), delinearizedInt(b));
            }

            // === RGB Cube Boundary Utilities (for bisectToLimit) ===

            // Compute CAM16 hue in radians from linear RGB.
            // Transforms via SCALED_DISCOUNT_FROM_LINRGB then applies chromatic adaptation.
            static auto hueOf(const std::array<double, 3>& linrgb) -> double {
                auto scaled = matrixMultiply(linrgb.data(), SCALED_DISCOUNT_FROM_LINRGB);
                double rA = chromaticAdaptation(scaled[0]);
                double gA = chromaticAdaptation(scaled[1]);
                double bA = chromaticAdaptation(scaled[2]);
                // Redness-greenness
                double a = (11.0 * rA + -12.0 * gA + bA) / 11.0;
                // Yellowness-blueness
                double b = (rA + gA - 2.0 * bA) / 9.0;
                return std::atan2(b, a);
            }

            // Check if angles a,b,c are in cyclic order on the circle.
            static auto areInCyclicOrder(double a, double b, double c) -> bool {
                double deltaAB = sanitizeRadians(b - a);
                double deltaAC = sanitizeRadians(c - a);
                return deltaAB < deltaAC;
            }

            // Solve lerp parameter t: lerp(source, target, t) = mid
            static auto intercept(double source, double mid, double target) -> double {
                return (mid - source) / (target - source);
            }

            // Linear interpolation between two 3D points
            static auto lerpPoint(const std::array<double, 3>& source, double t, const std::array<double, 3>& target) -> std::array<double, 3> {
                return std::array<double, 3>{source[0] + (target[0] - source[0]) * t, source[1] + (target[1] - source[1]) * t, source[2] + (target[2] - source[2]) * t,};
            }

            // Intersect segment AB with plane R=coord, G=coord, or B=coord.
            static auto setCoordinate(const std::array<double, 3>& source, double coordinate, const std::array<double, 3>& target, int axis) -> std::array<double, 3> {
                double t = intercept(source[axis], coordinate, target[axis]);
                return lerpPoint(source, t, target);
            }

            // Check if x is in [0, 100] (valid linear RGB range)
            static auto isBounded(double x) -> bool {
                return 0.0 <= x && x <= 100.0;
            }

            // Compute the nth vertex of the polygonal intersection of Y-plane with RGB cube.
            // Returns {-1, -1, -1} if the vertex lies outside the cube.
            // n in [0, 11]: 4 vertices per face x 3 faces (R, G, B free).
            static auto nthVertex(double y, int n) -> std::array<double, 3> {
                double kR = Y_FROM_LINRGB[0];
                double kG = Y_FROM_LINRGB[1];
                double kB = Y_FROM_LINRGB[2];
                double coordA = (n % 4 <= 1) ? 0.0 : 100.0;
                double coordB = (n % 2 == 0) ? 0.0 : 100.0;

                if (n < 4) {
                    double g = coordA;
                    double b = coordB;
                    double r = (y - g * kG - b * kB) / kR;
                    if (isBounded(r)) {
                        return {r, g, b};
                    }
                } else if (n < 8) {
                    double b = coordA;
                    double r = coordB;
                    double g = (y - r * kR - b * kB) / kG;
                    if (isBounded(g)) {
                        return {r, g, b};
                    }
                } else {
                    double r = coordA;
                    double g = coordB;
                    double b = (y - r * kR - g * kG) / kB;
                    if (isBounded(b)) {
                        return {r, g, b};
                    }
                }
                return {-1.0, -1.0, -1.0};
            }

            // Find the two vertices on the Y-plane polygon that bracket the target hue.
            // Returns via output parameters.
            static auto bisectToSegment(double y, double targetHue, std::array<double, 3>& outLeft, std::array<double, 3>& outRight) -> void {
                std::array<double, 3> left{-1.0, -1.0, -1.0};
                std::array<double, 3> right = left;
                double leftHue = 0.0;
                double rightHue = 0.0;
                bool initialized = false;
                bool uncut = true;

                for (int n = 0; n < 12; n++) {
                    auto mid = nthVertex(y, n);
                    if (mid[0] < 0) {
                        continue;
                    }
                    double midHue = hueOf(mid);
                    if (!initialized) {
                        left = mid;
                        right = mid;
                        leftHue = midHue;
                        rightHue = midHue;
                        initialized = true;
                        continue;
                    }
                    if (uncut || areInCyclicOrder(leftHue, midHue, rightHue)) {
                        uncut = false;
                        if (areInCyclicOrder(leftHue, targetHue, midHue)) {
                            right = mid;
                            rightHue = midHue;
                        } else {
                            left = mid;
                            leftHue = midHue;
                        }
                    }
                }
                outLeft = left;
                outRight = right;
            }

            // Midpoint of two 3D points.
            static auto midpoint(const std::array<double, 3>& a, const std::array<double, 3>& b) -> std::array<double, 3> {
                return std::array<double, 3>{(a[0] + b[0]) / 2.0, (a[1] + b[1]) / 2.0, (a[2] + b[2]) / 2.0,};
            }

            // Critical plane index helpers: map delinearized (0-255) to CRITICAL_PLANES indices.
            static auto criticalPlaneBelow(double x) -> int {
                return static_cast<int>(std::floor(x - 0.5));
            }

            static auto criticalPlaneAbove(double x) -> int {
                return static_cast<int>(std::ceil(x - 0.5));
            }

            // === Gamut Boundary Search ===
            // Given Y and target hue, find the point on the sRGB gamut boundary
            // that has that Y and hue. Returns linear RGB in [0, 100] range.

            static auto bisectToLimit(double y, double targetHue) -> std::array<double, 3> {
                std::array<double, 3> left, right;
                bisectToSegment(y, targetHue, left, right);

                double leftHue = hueOf(left);

                for (int axis = 0; axis < 3; axis++) {
                    if (left[axis] != right[axis]) {
                        int lPlane = -1;
                        int rPlane = 255;
                        if (left[axis] < right[axis]) {
                            lPlane = criticalPlaneBelow(trueDelinearized(left[axis]));
                            rPlane = criticalPlaneAbove(trueDelinearized(right[axis]));
                        } else {
                            lPlane = criticalPlaneAbove(trueDelinearized(left[axis]));
                            rPlane = criticalPlaneBelow(trueDelinearized(right[axis]));
                        }
                        for (int i = 0; i < 8; i++) {
                            if (std::abs(rPlane - lPlane) <= 1) {
                                break;
                            }
                            int mPlane = (lPlane + rPlane) / 2;
                            // Clamp to valid CRITICAL_PLANES range [0, 254]
                            mPlane = std::clamp(mPlane, 0, 254);
                            double midPlaneCoordinate = CRITICAL_PLANES[mPlane];
                            auto mid = setCoordinate(left, midPlaneCoordinate, right, axis);
                            double midHue = hueOf(mid);
                            if (areInCyclicOrder(leftHue, targetHue, midHue)) {
                                right = mid;
                                rPlane = mPlane;
                            } else {
                                left = mid;
                                leftHue = midHue;
                                lPlane = mPlane;
                            }
                        }
                    }
                }
                return midpoint(left, right);
            }

            // === Newton Iteration (core HCT solver) ===
            // Given hue (radians), chroma, and target Y, iteratively find J (CAM16
            // lightness) such that the forward transform (J, C, h) -> RGB -> Y
            // produces a Y value matching the target.
            //
            // Uses Newton's method with Jacobian approximation fn'(J) ~ 2*fn(J)/J.
            // Maximum 5 iterations, early exit if |fn(J) - Y| < 0.002.

            static auto findResultByJ(double hueRadians, double chroma, double y) -> std::int32_t {
                // Initial J estimate: approximate mapping from Y to CAM16 lightness
                double j = std::sqrt(y) * 11.0;

                auto& vc = ViewingConditions::DEFAULT;

                // Pre-compute iteration-invariant terms
                double tInnerCoeff = 1.0 / std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
                double eHue = 0.25 * (std::cos(hueRadians + 2.0) + 3.8);
                double p1 = eHue * (50000.0 / 13.0) * vc.nc * vc.ncb;
                double hSin = std::sin(hueRadians);
                double hCos = std::cos(hueRadians);

                for (int iterationRound = 0; iterationRound < NUM_ITERATIONS; iterationRound++) {
                    double jNormalized = j / 100.0;

                    // alpha = C / sqrt(J/100), with guard for degenerate cases
                    double alpha = (chroma == 0.0 || j == 0.0) ? 0.0 : chroma / std::sqrt(jNormalized);

                    // t (CAM16 eccentricity factor) from chroma
                    double t = std::pow(alpha * tInnerCoeff, 1.0 / 0.9);

                    // Achromatic response ac = aw * (J/100)^(1/(c*z))
                    double ac = vc.aw * std::pow(jNormalized, 1.0 / vc.c / vc.z);
                    double p2 = ac / vc.nbb;

                    // gamma relates achromatic/chromatic responses to opponent colors
                    double gamma = 23.0 * (p2 + 0.305) * t / (23.0 * p1 + 11.0 * t * hCos + 108.0 * t * hSin);

                    // Opponent color coordinates (a: red-green, b: yellow-blue)
                    double a = gamma * hCos;
                    double b = gamma * hSin;

                    // Inverse CAM16: opponent colors -> adapted cone responses
                    double rA = (460.0 * p2 + 451.0 * a + 288.0 * b) / 1403.0;
                    double gA = (460.0 * p2 - 891.0 * a - 261.0 * b) / 1403.0;
                    double bA = (460.0 * p2 - 220.0 * a - 6300.0 * b) / 1403.0;

                    // Inverse chromatic adaptation: adapted -> linear scaled cone responses
                    double rCScaled = inverseChromaticAdaptation(rA);
                    double gCScaled = inverseChromaticAdaptation(gA);
                    double bCScaled = inverseChromaticAdaptation(bA);

                    // Recover linear RGB from scaled discount space
                    double scaledRow[3] = {rCScaled, gCScaled, bCScaled};
                    auto linrgb = matrixMultiply(scaledRow, LINRGB_FROM_SCALED_DISCOUNT);

                    // Check for out-of-gamut result (negative means gamut boundary exceeded)
                    if (linrgb[0] < 0 || linrgb[1] < 0 || linrgb[2] < 0) {
                        return 0;
                    }

                    // Compute reconstructed Y from linear RGB
                    double kR = Y_FROM_LINRGB[0];
                    double kG = Y_FROM_LINRGB[1];
                    double kB = Y_FROM_LINRGB[2];
                    double fnj = kR * linrgb[0] + kG * linrgb[1] + kB * linrgb[2];

                    if (fnj <= 0) {
                        return 0;
                    }

                    // Check convergence or final iteration
                    if (iterationRound == 4 || std::abs(fnj - y) < 0.002) {
                        // Final validation: if any channel exceeds sRGB range, return 0
                        if (linrgb[0] > 100.01 || linrgb[1] > 100.01 || linrgb[2] > 100.01) {
                            return 0;
                        }
                        return argbFromLinrgb100(linrgb[0], linrgb[1], linrgb[2]);
                    }

                    // Newton step: J_{n+1} = J_n - (fn(J_n) - Y) * J_n / (2 * fn(J_n))
                    // Uses the approximation fn'(J) ~ 2*fn(J)/J
                    j = j - (fnj - y) * j / (2.0 * fnj);
                }
                return 0;
            }
        };

        // === HCT (Hue-Chroma-Tone) Color Space Wrapper ===
        // Thin wrapper over CAM16 for convenient HCT color representation.
        class Hct {
            double hue_ = 0, chroma_ = 0, tone_ = 0;
            std::int32_t argb_ = 0;
        public:
            Hct() = default;

            static auto fromInt(std::int32_t argb) -> Hct {
                auto cam = Cam16::fromArgb(argb);
                return Hct{cam.hue, cam.chroma, lstarFromArgb(argb), argb};
            }

            auto toInt() const -> std::int32_t {
                return argb_;
            }

            auto hue() const -> double {
                return hue_;
            }

            auto chroma() const -> double {
                return chroma_;
            }

            auto tone() const -> double {
                return tone_;
            }
        private:
            Hct(double h, double c, double t, std::int32_t a) : hue_(h),
                                                                chroma_(c),
                                                                tone_(t),
                                                                argb_(a) {}
        };

        // === TonalPalette ===
        // Maps a (hue, chroma) pair to a set of colors at varying tones (0-100).
        // Ported from material_color_utilities palette_ts.dart.
        class TonalPalette {
            double hue_ = 0, chroma_ = 0;
        public:
            TonalPalette() = default;

            static auto fromHueAndChroma(double hue, double chroma) -> TonalPalette {
                return TonalPalette{hue, chroma};
            }

            auto tone(double t) const -> type::Color {
                double adjustedHue = hue_;
                // Yellow T99 fix: yellow hues (70-120°) at T99 use average of T98+T100
                if (std::abs(t - 99.0) < 0.5 && adjustedHue >= 70.0 && adjustedHue <= 120.0) {
                    auto c98 = HctSolver::solveToInt(adjustedHue, chroma_, 98.0);
                    auto c100 = HctSolver::solveToInt(adjustedHue, chroma_, 100.0);
                    auto avg = argbFromRgb((redFromArgb(c98) + redFromArgb(c100)) / 2, (greenFromArgb(c98) + greenFromArgb(c100)) / 2, (blueFromArgb(c98) + blueFromArgb(c100)) / 2);
                    return toColor(avg);
                }
                auto argb = HctSolver::solveToInt(adjustedHue, chroma_, t);
                return toColor(argb);
            }

            auto hue() const -> double {
                return hue_;
            }

            auto chroma() const -> double {
                return chroma_;
            }
        private:
            TonalPalette(double h, double c) : hue_(h),
                                               chroma_(c) {}

            static auto toColor(std::int32_t argb) -> type::Color {
                return type::Color(redFromArgb(argb), greenFromArgb(argb), blueFromArgb(argb), alphaFromArgb(argb));
            }
        };

        // === DynamicScheme (tonalSpot) ===
        // Holds the 6 tonal palettes for a tonalSpot color scheme variant
        // alongside isDark / contrastLevel flags.
        class DynamicScheme {
        public:
            Hct sourceHct_;
            TonalPalette primaryPalette_, secondaryPalette_, tertiaryPalette_;
            TonalPalette neutralPalette_, neutralVariantPalette_, errorPalette_;
            bool isDark_;
            double contrastLevel_;

            DynamicScheme(Hct source, bool isDark, double contrastLevel) : sourceHct_(source),
                                                                           isDark_(isDark),
                                                                           contrastLevel_(contrastLevel) {
                auto h = source.hue();
                primaryPalette_ = TonalPalette::fromHueAndChroma(h, 36.0);
                secondaryPalette_ = TonalPalette::fromHueAndChroma(h, 16.0);
                tertiaryPalette_ = TonalPalette::fromHueAndChroma(sanitizeDegrees(h + 60.0), 24.0);
                neutralPalette_ = TonalPalette::fromHueAndChroma(h, 6.0);
                neutralVariantPalette_ = TonalPalette::fromHueAndChroma(h, 8.0);
                errorPalette_ = TonalPalette::fromHueAndChroma(25.0, 84.0);
            }
        };

        // === ContrastCurve ===
        // Interpolates contrast level values at standard breakpoints:
        //   cl = -1.0 -> low
        //   cl =  0.0 -> normal
        //   cl =  1.0 -> high
        //   cl =  2.0 -> highest
        struct ContrastCurve {
            double low, normal, high, highest;

            auto get(double cl) const -> double {
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

        // === Polarity for ToneDeltaPair ===
        enum class Polarity {
            Nearer,
            Lighter,
            Darker,
            NoPreference
        };

        // === ToneDeltaPair ===
        struct ToneDeltaPair {
            const DynamicColor* roleA;
            const DynamicColor* roleB;
            double delta;
            Polarity polarity;
            bool stayTogether;
        };

        // === DynamicColor ===
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

            DynamicColor(PaletteFn p, ToneFn t, BgFn b, ContrastCurve cc, std::optional<ToneDeltaPair> pr = std::nullopt, bool elf = true) : paletteFn_(p),
                toneFn_(t),
                bgFn_(b),
                curve_(cc),
                pair_(pr),
                enableLightFg_(elf) {}

            auto getArgb(const DynamicScheme& s) const -> std::int32_t;
            auto getTone(const DynamicScheme& s) const -> double;

            auto getPalette(const DynamicScheme& s) const -> const TonalPalette& {
                return paletteFn_(s);
            }
        };

        // === WCAG contrast ratio ===
        inline auto ratioOfTones(double ta, double tb) -> double {
            auto ya = yFromLstar(ta), yb = yFromLstar(tb);
            auto l = std::max(ya, yb), d = std::min(ya, yb);
            return (l + 0.05) / (d + 0.05);
        }

        // === Find foreground tone meeting target contrast ratio against background ===
        inline auto foregroundTone(double bgTone, double targetRatio) -> double {
            auto lightFg = [&] {
                return 100.0;
            };
            auto darkFg = [&] {
                return 0.0;
            };
            if (ratioOfTones(100.0, bgTone) >= targetRatio) {
                return 100.0;
            }
            if (ratioOfTones(0.0, bgTone) >= targetRatio) {
                return 0.0;
            }
            // Pick whichever is closer (MCU prefers light foreground when possible)
            return (ratioOfTones(100.0, bgTone) >= ratioOfTones(0.0, bgTone)) ? 100.0 : 0.0;
        }

        // === noBg: placeholder for roles with enableLightForeground=false ===
        // Returns a const reference to a default-constructed DynamicColor.
        // When used as a BgFn, getTone() will never be called on it because
        // enableLightFg_ is false for the foreground role.
        inline auto noBg(const DynamicScheme&) -> const DynamicColor& {
            static DynamicColor dummy;
            return dummy;
        }

        // === ARGB resolution ===
        inline auto DynamicColor::getArgb(const DynamicScheme& s) const -> std::int32_t {
            auto color = getPalette(s).tone(getTone(s));
            return argbFromRgb(color.r, color.g, color.b);
        }

        // === Full tone resolution pipeline ===
        inline auto DynamicColor::getTone(const DynamicScheme& s) const -> double {
            auto tone = toneFn_(s);
            if (enableLightFg_) {
                auto bg = bgFn_(s).getTone(s);
                auto target = curve_.get(s.contrastLevel_);
                tone = foregroundTone(bg, target);
            }
            if (pair_.has_value()) {
                auto& p = pair_.value();
                auto tB = p.roleB->getTone(s);
                switch (p.polarity) {
                    case Polarity::Nearer:
                        if (std::abs(tone - tB) < p.delta) {
                            tone = (tone < tB) ? (tB - p.delta) : (tB + p.delta);
                        }
                        break;
                    case Polarity::Lighter:
                        if (tone < tB + p.delta) {
                            tone = tB + p.delta;
                        }
                        break;
                    case Polarity::Darker:
                        if (tone > tB - p.delta) {
                            tone = tB - p.delta;
                        }
                        break;
                    case Polarity::NoPreference:
                        if (std::abs(tone - tB) < p.delta) {
                            auto dLight = (tone + p.delta) - tB;
                            auto dDark = tB - (tone - p.delta);
                            tone = (dLight < dDark) ? (tB - p.delta) : (tB + p.delta);
                        }
                        break;
                }
                tone = std::clamp(tone, 0.0, 100.0);
            }
            return tone;
        }

        // ─── MaterialDynamicColors ─────────────────────────────
        // All color role definitions for `tonalSpot` scheme variant.
        // Uses `contrastLevel=0.0` as base; `getTone` applies contrast curves dynamically.
        struct MaterialDynamicColors {
            // Palette accessors
            static auto priPal(const DynamicScheme& s) -> const TonalPalette& {
                return s.primaryPalette_;
            }

            static auto secPal(const DynamicScheme& s) -> const TonalPalette& {
                return s.secondaryPalette_;
            }

            static auto terPal(const DynamicScheme& s) -> const TonalPalette& {
                return s.tertiaryPalette_;
            }

            static auto neuPal(const DynamicScheme& s) -> const TonalPalette& {
                return s.neutralPalette_;
            }

            static auto nvpPal(const DynamicScheme& s) -> const TonalPalette& {
                return s.neutralVariantPalette_;
            }

            static auto errPal(const DynamicScheme& s) -> const TonalPalette& {
                return s.errorPalette_;
            }

            // Tone accessors
            static auto tone40_80(const DynamicScheme& s) -> double {
                return s.isDark_ ? 80.0 : 40.0;
            }

            static auto tone100_20(const DynamicScheme& s) -> double {
                return s.isDark_ ? 20.0 : 100.0;
            }

            static auto tone90_30(const DynamicScheme& s) -> double {
                return s.isDark_ ? 30.0 : 90.0;
            }

            static auto tone10_90(const DynamicScheme& s) -> double {
                return s.isDark_ ? 90.0 : 10.0;
            }

            static auto tone90_90(const DynamicScheme&) -> double {
                return 90.0;
            }

            static auto tone80_80(const DynamicScheme&) -> double {
                return 80.0;
            }

            static auto tone80_40(const DynamicScheme& s) -> double {
                return s.isDark_ ? 40.0 : 80.0;
            }

            static auto tone10_10(const DynamicScheme&) -> double {
                return 10.0;
            }

            static auto tone30_30(const DynamicScheme&) -> double {
                return 30.0;
            }

            static auto tone30_80(const DynamicScheme& s) -> double {
                return s.isDark_ ? 80.0 : 30.0;
            }

            static auto tone98_6(const DynamicScheme& s) -> double {
                return s.isDark_ ? 6.0 : 98.0;
            }

            static auto tone87_6(const DynamicScheme& s) -> double {
                return s.isDark_ ? 6.0 : 87.0;
            }

            static auto tone98_24(const DynamicScheme& s) -> double {
                return s.isDark_ ? 24.0 : 98.0;
            }

            static auto tone100_4(const DynamicScheme& s) -> double {
                return s.isDark_ ? 4.0 : 100.0;
            }

            static auto tone96_10(const DynamicScheme& s) -> double {
                return s.isDark_ ? 10.0 : 96.0;
            }

            static auto tone94_12(const DynamicScheme& s) -> double {
                return s.isDark_ ? 12.0 : 94.0;
            }

            static auto tone92_22(const DynamicScheme& s) -> double {
                return s.isDark_ ? 22.0 : 92.0;
            }

            static auto tone90_24(const DynamicScheme& s) -> double {
                return s.isDark_ ? 24.0 : 90.0;
            }

            static auto tone50_60(const DynamicScheme& s) -> double {
                return s.isDark_ ? 60.0 : 50.0;
            }

            static auto tone80_30(const DynamicScheme& s) -> double {
                return s.isDark_ ? 30.0 : 80.0;
            }

            static auto tone20_90(const DynamicScheme& s) -> double {
                return s.isDark_ ? 90.0 : 20.0;
            }

            static auto tone95_20(const DynamicScheme& s) -> double {
                return s.isDark_ ? 20.0 : 95.0;
            }

            static auto tone0_0(const DynamicScheme&) -> double {
                return 0.0;
            }

            // Background resolver for contrast: highest surface
            // Defined as standalone static to avoid circular dependency
            // (enableLightForeground=false since it IS a surface, not a foreground on it)
            static auto highestSurface(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone90_24, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            // ─── Primary group ───
            static auto primary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone40_80, highestSurface, ContrastCurve{3, 4.5, 7, 7});
                return inst;
            }

            static auto onPrimary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone100_20, primary, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto primaryContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone90_30, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onPrimaryContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone10_90, primaryContainer, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto primaryFixed(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone90_90, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto primaryFixedDim(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone80_80, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onPrimaryFixed(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone10_10, primaryFixedDim, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto onPrimaryFixedVariant(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone30_30, primaryFixedDim, ContrastCurve{3, 4.5, 7, 11});
                return inst;
            }

            // ─── Secondary group ───
            static auto secondary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone40_80, highestSurface, ContrastCurve{3, 4.5, 7, 7});
                return inst;
            }

            static auto onSecondary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone100_20, secondary, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto secondaryContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone90_30, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onSecondaryContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone10_90, secondaryContainer, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto secondaryFixed(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone90_90, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto secondaryFixedDim(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone80_80, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onSecondaryFixed(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone10_10, secondaryFixedDim, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto onSecondaryFixedVariant(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(secPal, tone30_30, secondaryFixedDim, ContrastCurve{3, 4.5, 7, 11});
                return inst;
            }

            // ─── Tertiary group ───
            static auto tertiary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone40_80, highestSurface, ContrastCurve{3, 4.5, 7, 7});
                return inst;
            }

            static auto onTertiary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone100_20, tertiary, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto tertiaryContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone90_30, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onTertiaryContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone10_90, tertiaryContainer, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto tertiaryFixed(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone90_90, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto tertiaryFixedDim(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone80_80, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onTertiaryFixed(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone10_10, tertiaryFixedDim, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto onTertiaryFixedVariant(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(terPal, tone30_30, tertiaryFixedDim, ContrastCurve{3, 4.5, 7, 11});
                return inst;
            }

            // ─── Error group ───
            static auto error(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(errPal, tone40_80, highestSurface, ContrastCurve{3, 4.5, 7, 7});
                return inst;
            }

            static auto onError(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(errPal, tone100_20, error, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto errorContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(errPal, tone90_30, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            static auto onErrorContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(errPal, tone10_90, errorContainer, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            // ─── Surface group (all enableLightForeground=false — they ARE surfaces) ───
            static auto surface(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone98_6, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceDim(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone87_6, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceBright(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone98_24, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceContainerLowest(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone100_4, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceContainerLow(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone96_10, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceContainer(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone94_12, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceContainerHigh(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone92_22, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto surfaceContainerHighest(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone90_24, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto onSurface(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone10_90, highestSurface, ContrastCurve{4.5, 7, 11, 21});
                return inst;
            }

            static auto surfaceVariant(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(nvpPal, tone90_30, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto onSurfaceVariant(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(nvpPal, tone30_80, highestSurface, ContrastCurve{3, 4.5, 7, 11});
                return inst;
            }

            // ─── Outline ───
            static auto outline(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(nvpPal, tone50_60, highestSurface, ContrastCurve{1.5, 3, 4.5, 7});
                return inst;
            }

            static auto outlineVariant(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(nvpPal, tone80_30, highestSurface, ContrastCurve{1, 1, 3, 4.5});
                return inst;
            }

            // ─── Inverse ───
            static auto inverseSurface(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone20_90, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto inverseOnSurface(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone95_20, inverseSurface, ContrastCurve{3, 4.5, 7, 11});
                return inst;
            }

            static auto inversePrimary(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(priPal, tone80_40, inverseSurface, ContrastCurve{3, 4.5, 7, 7});
                return inst;
            }

            // ─── Other ───
            static auto shadow(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone0_0, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }

            static auto scrim(const DynamicScheme&) -> const DynamicColor& {
                static DynamicColor inst(neuPal, tone0_0, noBg, ContrastCurve{0, 0, 0, 0}, std::nullopt, false);
                return inst;
            }
        };
    } // namespace detail

    struct ColorScheme {
        type::Color primary;
        type::Color onPrimary;
        type::Color primaryContainer;
        type::Color onPrimaryContainer;
        type::Color primaryFixed;
        type::Color primaryFixedDim;
        type::Color onPrimaryFixed;
        type::Color onPrimaryFixedVariant;
        type::Color secondary;
        type::Color onSecondary;
        type::Color secondaryContainer;
        type::Color onSecondaryContainer;
        type::Color secondaryFixed;
        type::Color secondaryFixedDim;
        type::Color onSecondaryFixed;
        type::Color onSecondaryFixedVariant;
        type::Color tertiary;
        type::Color onTertiary;
        type::Color tertiaryContainer;
        type::Color onTertiaryContainer;
        type::Color tertiaryFixed;
        type::Color tertiaryFixedDim;
        type::Color onTertiaryFixed;
        type::Color onTertiaryFixedVariant;
        type::Color error;
        type::Color onError;
        type::Color errorContainer;
        type::Color onErrorContainer;
        type::Color surface;
        type::Color surfaceDim;
        type::Color surfaceBright;
        type::Color surfaceContainerLowest;
        type::Color surfaceContainerLow;
        type::Color surfaceContainer;
        type::Color surfaceContainerHigh;
        type::Color surfaceContainerHighest;
        type::Color onSurface;
        type::Color surfaceVariant;
        type::Color onSurfaceVariant;
        type::Color outline;
        type::Color outlineVariant;
        type::Color inverseSurface;
        type::Color inverseOnSurface;
        type::Color inversePrimary;
        type::Color shadow;
        type::Color scrim;

        static auto fromSeed(type::Color seedColor, const bool isDark = false, const float contrast_level = 0.0F) -> ColorScheme {
            auto argb = detail::argbFromRgb(seedColor.r, seedColor.g, seedColor.b);
            auto hct = detail::Hct::fromInt(argb);
            detail::DynamicScheme scheme(hct, isDark, contrast_level);

            auto resolve = [&](const detail::DynamicColor& role) -> type::Color {
                auto a = role.getArgb(scheme);
                return {detail::redFromArgb(a), detail::greenFromArgb(a), detail::blueFromArgb(a), detail::alphaFromArgb(a)};
            };

            return {
                .primary = resolve(detail::MaterialDynamicColors::primary(scheme)),
                .onPrimary = resolve(detail::MaterialDynamicColors::onPrimary(scheme)),
                .primaryContainer = resolve(detail::MaterialDynamicColors::primaryContainer(scheme)),
                .onPrimaryContainer = resolve(detail::MaterialDynamicColors::onPrimaryContainer(scheme)),
                .primaryFixed = resolve(detail::MaterialDynamicColors::primaryFixed(scheme)),
                .primaryFixedDim = resolve(detail::MaterialDynamicColors::primaryFixedDim(scheme)),
                .onPrimaryFixed = resolve(detail::MaterialDynamicColors::onPrimaryFixed(scheme)),
                .onPrimaryFixedVariant = resolve(detail::MaterialDynamicColors::onPrimaryFixedVariant(scheme)),
                .secondary = resolve(detail::MaterialDynamicColors::secondary(scheme)),
                .onSecondary = resolve(detail::MaterialDynamicColors::onSecondary(scheme)),
                .secondaryContainer = resolve(detail::MaterialDynamicColors::secondaryContainer(scheme)),
                .onSecondaryContainer = resolve(detail::MaterialDynamicColors::onSecondaryContainer(scheme)),
                .secondaryFixed = resolve(detail::MaterialDynamicColors::secondaryFixed(scheme)),
                .secondaryFixedDim = resolve(detail::MaterialDynamicColors::secondaryFixedDim(scheme)),
                .onSecondaryFixed = resolve(detail::MaterialDynamicColors::onSecondaryFixed(scheme)),
                .onSecondaryFixedVariant = resolve(detail::MaterialDynamicColors::onSecondaryFixedVariant(scheme)),
                .tertiary = resolve(detail::MaterialDynamicColors::tertiary(scheme)),
                .onTertiary = resolve(detail::MaterialDynamicColors::onTertiary(scheme)),
                .tertiaryContainer = resolve(detail::MaterialDynamicColors::tertiaryContainer(scheme)),
                .onTertiaryContainer = resolve(detail::MaterialDynamicColors::onTertiaryContainer(scheme)),
                .tertiaryFixed = resolve(detail::MaterialDynamicColors::tertiaryFixed(scheme)),
                .tertiaryFixedDim = resolve(detail::MaterialDynamicColors::tertiaryFixedDim(scheme)),
                .onTertiaryFixed = resolve(detail::MaterialDynamicColors::onTertiaryFixed(scheme)),
                .onTertiaryFixedVariant = resolve(detail::MaterialDynamicColors::onTertiaryFixedVariant(scheme)),
                .error = resolve(detail::MaterialDynamicColors::error(scheme)),
                .onError = resolve(detail::MaterialDynamicColors::onError(scheme)),
                .errorContainer = resolve(detail::MaterialDynamicColors::errorContainer(scheme)),
                .onErrorContainer = resolve(detail::MaterialDynamicColors::onErrorContainer(scheme)),
                .surface = resolve(detail::MaterialDynamicColors::surface(scheme)),
                .surfaceDim = resolve(detail::MaterialDynamicColors::surfaceDim(scheme)),
                .surfaceBright = resolve(detail::MaterialDynamicColors::surfaceBright(scheme)),
                .surfaceContainerLowest = resolve(detail::MaterialDynamicColors::surfaceContainerLowest(scheme)),
                .surfaceContainerLow = resolve(detail::MaterialDynamicColors::surfaceContainerLow(scheme)),
                .surfaceContainer = resolve(detail::MaterialDynamicColors::surfaceContainer(scheme)),
                .surfaceContainerHigh = resolve(detail::MaterialDynamicColors::surfaceContainerHigh(scheme)),
                .surfaceContainerHighest = resolve(detail::MaterialDynamicColors::surfaceContainerHighest(scheme)),
                .onSurface = resolve(detail::MaterialDynamicColors::onSurface(scheme)),
                .surfaceVariant = resolve(detail::MaterialDynamicColors::surfaceVariant(scheme)),
                .onSurfaceVariant = resolve(detail::MaterialDynamicColors::onSurfaceVariant(scheme)),
                .outline = resolve(detail::MaterialDynamicColors::outline(scheme)),
                .outlineVariant = resolve(detail::MaterialDynamicColors::outlineVariant(scheme)),
                .inverseSurface = resolve(detail::MaterialDynamicColors::inverseSurface(scheme)),
                .inverseOnSurface = resolve(detail::MaterialDynamicColors::inverseOnSurface(scheme)),
                .inversePrimary = resolve(detail::MaterialDynamicColors::inversePrimary(scheme)),
                .shadow = resolve(detail::MaterialDynamicColors::shadow(scheme)),
                .scrim = resolve(detail::MaterialDynamicColors::scrim(scheme)),
            };
        }
    };
} // namespace neko::seed
