#pragma once
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <optional>
#include <glm/glm.hpp>
#include "NekoUI/Type.hpp"

namespace neko::seed {

struct ColorScheme;  // forward decl

namespace detail {

// === ARGB Utility ===
constexpr auto argbFromRgb(int r, int g, int b) -> std::int32_t {
    return (255 << 24) | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
}
constexpr auto alphaFromArgb(std::int32_t a) -> int { return (a >> 24) & 0xFF; }
constexpr auto redFromArgb(std::int32_t a)   -> int { return (a >> 16) & 0xFF; }
constexpr auto greenFromArgb(std::int32_t a) -> int { return (a >> 8) & 0xFF; }
constexpr auto blueFromArgb(std::int32_t a)  -> int { return a & 0xFF; }

// === sRGB Gamma ===
inline auto linearized(int rgbComponent) -> double {
    double n = rgbComponent / 255.0;
    return (n <= 0.040449936) ? (n / 12.92) : std::pow((n + 0.055) / 1.055, 2.4);
}
inline auto delinearized(double rgbComponent) -> int {
    double n;
    if (rgbComponent <= 0.0031308) n = rgbComponent * 12.92;
    else n = 1.055 * std::pow(rgbComponent, 1.0 / 2.4) - 0.055;
    return static_cast<int>(std::round(n * 255.0));
}
inline auto argbFromLinrgb(double r, double g, double b) -> std::int32_t {
    return argbFromRgb(delinearized(r), delinearized(g), delinearized(b));
}

// === L* ↔ Y (CIE Lightness ↔ Relative Luminance) ===
inline auto yFromLstar(double lstar) -> double {
    return (lstar > 8.0)
        ? std::pow((lstar + 16.0) / 116.0, 3.0)
        : lstar / 903.3;
}
inline auto lstarFromY(double y) -> double {
    return (y > 0.008856)
        ? 116.0 * std::cbrt(y) - 16.0
        : y * 903.3;
}
inline auto lstarFromArgb(std::int32_t argb) -> double {
    auto y = 0.2126 * linearized(redFromArgb(argb))
           + 0.7152 * linearized(greenFromArgb(argb))
           + 0.0722 * linearized(blueFromArgb(argb));
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

// === ViewingConditions (CAM16 viewing conditions for D65) ===
struct ViewingConditions {
    double n, aw, nbb, ncb, c, nc, fl, fLRoot, z;

    static auto make() -> ViewingConditions {
        double whitePoint[] = {0.95047, 1.0, 1.08883};
        double adaptingLuminance = 200.0 / 3.141592653589793 * 0.2;
        double backgroundLstar = 50.0;
        double surround = 2.0; // average

        auto yw = whitePoint[1];

        double c, nc, f;
        if (std::abs(surround - 2.0) < 0.1) { c = 0.69; nc = 1.0; f = 1.0; }
        else if (std::abs(surround - 1.0) < 0.1) { c = 0.59; nc = 0.9; f = 0.9; }
        else { c = 0.525; nc = 0.8; f = 0.8; }

        auto nVal = yFromLstar(backgroundLstar) / yw;
        auto nbb = std::pow(nVal, 0.2);
        auto ncb = nbb;

        double d = f * (1.0 - (1.0 / 3.6) * std::exp((-adaptingLuminance - 42.0) / 92.0));
        d = std::clamp(d, 0.0, 1.0);

        auto flL = adaptingLuminance * 0.2;
        flL = flL * std::pow(0.9 / flL, 0.5);
        auto flRoot = std::pow(flL, 0.25);

        // Compute aw for D65 white
        double xW = whitePoint[0], zW = whitePoint[2];
        double rC = xW * 0.401288 + yw * 0.650173 + zW * -0.051461;
        double gC = xW * -0.250268 + yw * 1.204414 + zW * 0.045854;
        double bC = xW * -0.002079 + yw * 0.048952 + zW * 0.953127;

        double rD = rC * (yw * d / rC + 1.0 - d);
        double gD = gC * (yw * d / gC + 1.0 - d);
        double bD = bC * (yw * d / bC + 1.0 - d);

        double rP = std::cbrt(rD / 100.0) * 100.0;
        double gP = std::cbrt(gD / 100.0) * 100.0;
        double bP = std::cbrt(bD / 100.0) * 100.0;

        double aw = nbb * (2.0 * rP + gP + 0.05 * bP);
        double z = 1.48 + std::sqrt(nVal);

        return {nVal, aw, nbb, ncb, c, nc, flL, flRoot, z};
    }
    static ViewingConditions DEFAULT;
};
inline ViewingConditions ViewingConditions::DEFAULT = ViewingConditions::make();

// === CAM16 forward transform ===
struct Cam16 {
    double hue = 0.0, chroma = 0.0, j = 0.0, q = 0.0, m = 0.0, s = 0.0, jstar = 0.0;

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

        double rD = rC, gD = gC, bD = bC; // D65 CAT02 ~ identity

        double rP = std::cbrt(rD / 100.0) * 100.0;
        double gP = std::cbrt(gD / 100.0) * 100.0;
        double bP = std::cbrt(bD / 100.0) * 100.0;

        double a = vc.nbb * (2.0 * rP + gP + 0.05 * bP);
        double j = 100.0 * std::pow(a / vc.aw, vc.c * vc.z);

        double hRad = std::atan2(bP - gP, rP - gP);
        double hDeg = hRad * 180.0 / 3.141592653589793;
        if (hDeg < 0) hDeg += 360.0;

        double hRadians = hDeg * 3.141592653589793 / 180.0;
        double eT = 0.25 * (std::cos(hRadians + 2.0) + 3.8);

        double tBase = 50000.0 / 13.0 * vc.nc * vc.ncb * eT;
        double tNum = std::sqrt(rP * rP + gP * gP - 2.0 * rP * gP - rP * bP + gP * bP);
        double tDen = rP + gP + 1.05 * bP + 0.305;
        double t = tBase * tNum / tDen;

        double alpha = std::pow(t, 0.9) * std::pow(1.64 - std::pow(0.29, vc.n), 0.73);
        double chroma = alpha * std::sqrt(j / 100.0);

        return {hDeg, chroma, j, 0.0, 0.0, 0.0, 0.0};
    }
};
} // namespace detail
} // namespace neko::seed
