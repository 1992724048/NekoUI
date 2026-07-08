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
} // namespace detail
} // namespace neko::seed
