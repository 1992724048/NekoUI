# ColorSeed — Flutter-Style Google Color Seed System

**Date**: 2026-07-08
**Project**: NekoUI
**Status**: Approved design

## Overview

Implement `ColorScheme::fromSeed()` following Flutter's `material_color_utilities` (MCU) algorithm. Given a single seed color, generate a complete Material Design 3 color scheme using the HCT (Hue-Chroma-Tone) perceptual color space.

## Requirements

| Item | Value |
|------|-------|
| Fidelity | Exact MCU match (full HCT/CAM16) |
| Scheme variant | `tonalSpot` (Flutter default) |
| Light/Dark | Both modes |
| Color roles | All 30+ (surfaceContainer*, fixed*, inverse*, outline*, shadow, scrim) |
| Contrast level | Full `contrastLevel` curve support (-1.0 to 1.0) |
| Build output | Single header file: `Engine/Component/ColorSeed.hpp` |
| Color type | `neko::type::Color` = `glm::ivec4` (RGBA 0-255) |

## Public API

```cpp
namespace neko::seed {

struct ColorScheme {
    // Primary group
    type::Color primary;
    type::Color onPrimary;
    type::Color primaryContainer;
    type::Color onPrimaryContainer;
    type::Color primaryFixed;
    type::Color primaryFixedDim;
    type::Color onPrimaryFixed;
    type::Color onPrimaryFixedVariant;

    // Secondary group (same 8 roles)
    // Tertiary group (same 8 roles)

    // Error group
    type::Color error;
    type::Color onError;
    type::Color errorContainer;
    type::Color onErrorContainer;

    // Surface / Neutral
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

    // Outline
    type::Color outline;
    type::Color outlineVariant;

    // Inverse
    type::Color inverseSurface;
    type::Color inverseOnSurface;
    type::Color inversePrimary;

    // Other
    type::Color shadow;
    type::Color scrim;

    static auto fromSeed(
        type::Color seedColor,
        bool isDark = false,
        float contrastLevel = 0.0f
    ) -> ColorScheme;
};

} // namespace neko::seed
```

## Internal Architecture (single header file)

All implementation lives in `neko::seed::detail` namespace, hidden from API consumers.

### Layer 1: CAM16 Color Appearance Model (`detail::Cam16`)

Port of `material_color_utilities`'s `cam16.ts` / `hct_solver.ts`.

Key conversions:

```
ARGB → linear RGB (inverse sRGB gamma)
linear RGB → XYZ (D65, 2° observer)
XYZ → CAM16 (J, C, h, Q, M, s) via the CAM16 color appearance model
CAM16 → XYZ → linear RGB → ARGB (inverse)
```

**ViewingConditions**: D65 illuminant, 200 lux, gray surround (matching MCU defaults).

The inverse solver (`HctSolver.solveToInt`) uses Newton iteration on CAM16's `J` (lightness) to converge to the target L* (tone):

```
function solveToInt(hueDeg, chroma, lstar):
    hue = hueDeg * π/180
    while not converged (max 5 iterations):
        J = yFromLstar(lstar)  → L* to relative luminance Y → CAM16 J
        CAM16 from J, hue, chroma
        J差异 = J - targetJ
        adjust J via Newton method
    return CAM16 → ARGB
```

If exact chroma is unreachable at the target tone, fall back to `bisectToLimit` which finds the maximum reachable chroma.

### Layer 2: HCT Color Space (`detail::Hct`)

```
Hct(hue: 0-360°, chroma: 0-~200, tone: 0-100) → ARGB
Hct::fromInt(ARGB) → Hct (extracts hue, chroma, tone from ARGB)
```

The HCT space combines:
- **Hue** = CAM16 hue angle (0-360°)
- **Chroma** = CAM16 chroma
- **Tone** = CIE L* (perceptual lightness, 0-100)

Helper functions:
- `yFromLstar(lstar)` / `lstarFromY(y)`: L* ↔ relative luminance Y
- `argbFromLstar(lstar)`: gray ARGB at given lightness

### Layer 3: Tonal Palette (`detail::TonalPalette`)

A tonal palette is a set of colors at constant **hue** and **chroma**, varying only in **tone**.

```cpp
class TonalPalette {
    double hue_;
    double chroma_;
    int keyColor_;  // ARGB of the key color

public:
    // Factory: creates from hue+chroma, finds key color
    static auto fromHueAndChroma(double hue, double chroma) -> TonalPalette;
    static auto fromHct(const Hct& hct) -> TonalPalette;

    // Get ARGB for a given tone (0-100)
    auto tone(double tone) const -> type::Color;
};
```

**KeyColor binary search**: binary search through tones 0-100 to find the tone closest to T50 that achieves ≥ requested chroma (within ε=0.01). This is the "representative" color of the palette.

**Yellow T99 fix**: Yellow hues experience a CAM16 discontinuity at T99 → average T98 and T100 ARGB values.

### Layer 4: DynamicScheme (`detail::DynamicScheme`)

Holds the 6 tonal palettes for a scheme variant + mode + contrast level.

For **tonalSpot** (the default):

| Palette | Hue | Chroma |
|---------|-----|--------|
| primary | source.hue | 36.0 |
| secondary | source.hue | 16.0 |
| tertiary | source.hue + 60° | 24.0 |
| neutral | source.hue | 6.0 |
| neutralVariant | source.hue | 8.0 |
| error | 25.0 | 84.0 |

```cpp
class DynamicScheme {
    Hct sourceColorHct_;
    TonalPalette primaryPalette_;
    TonalPalette secondaryPalette_;
    TonalPalette tertiaryPalette_;
    TonalPalette neutralPalette_;
    TonalPalette neutralVariantPalette_;
    TonalPalette errorPalette_;
    bool isDark_;
    double contrastLevel_;

public:
    DynamicScheme(Hct source, bool isDark, double contrastLevel);
};
```

### Layer 5: MaterialDynamicColors — Tone Resolver

Resolution pipeline for each color role:

1. **Lookup base tone**: light/dark tone from the role's tone table
2. **Background contrast**: if the role has a background role, compute `foregroundTone(bgTone, targetRatio)` to ensure WCAG contrast
3. **ContrastCurve interpolation**: target ratio = `Curve.get(contrastLevel)`
4. **ToneDeltaPair**: enforce minimum tone difference from a paired role
5. **enableLightForeground**: if tone < 60 but can't reach 4.5:1, bump to 49

**ContrastCurve**:

```cpp
struct ContrastCurve {
    double low;      // contrastLevel = -1.0
    double normal;   // contrastLevel = 0.0
    double high;     // contrastLevel = 1.0
    double highest;  // contrastLevel = 2.0+ (not used by Flutter)

    double get(double contrastLevel) const;
};
```

**ToneDeltaPair**:

```cpp
enum class Polarity { Nearer, Lighter, Darker, NoPreference };

struct ToneDeltaPair {
    const DynamicColor* roleA;
    const DynamicColor* roleB;
    double delta;
    Polarity polarity;
    bool stayTogether;
};
```

### Layer 6: Tone Mapping Table

Complete tone mapping for `contrastLevel=0.0` (simplified; actual values use contrast curves):

| Role | Palette | Light Tone | Dark Tone |
|------|---------|:----------:|:---------:|
| primary | primary | 40 | 80 |
| onPrimary | primary | 100 | 20 |
| primaryContainer | primary | 90 | 30 |
| onPrimaryContainer | primary | 10 | 90 |
| primaryFixed | primary | 90 | 90 |
| primaryFixedDim | primary | 80 | 80 |
| onPrimaryFixed | primary | 10 | 10 |
| onPrimaryFixedVariant | primary | 30 | 30 |
| secondary | secondary | 40 | 80 |
| onSecondary | secondary | 100 | 20 |
| secondaryContainer | secondary | 90 | 30 |
| onSecondaryContainer | secondary | 10 | 90 |
| tertiary | tertiary | 40 | 80 |
| onTertiary | tertiary | 100 | 20 |
| tertiaryContainer | tertiary | 90 | 30 |
| onTertiaryContainer | tertiary | 10 | 90 |
| error | error | 40 | 80 |
| onError | error | 100 | 20 |
| errorContainer | error | 90 | 30 |
| onErrorContainer | error | 10 | 90 |
| surface | neutral | 98 | 6 |
| surfaceDim | neutral | 87 | 6 |
| surfaceBright | neutral | 98 | 24 |
| surfaceContainerLowest | neutral | 100 | 4 |
| surfaceContainerLow | neutral | 96 | 10 |
| surfaceContainer | neutral | 94 | 12 |
| surfaceContainerHigh | neutral | 92 | 22 |
| surfaceContainerHighest | neutral | 90 | 24 |
| onSurface | neutral | 10 | 90 |
| surfaceVariant | neutralVariant | 90 | 30 |
| onSurfaceVariant | neutralVariant | 30 | 80 |
| outline | neutralVariant | 50 | 60 |
| outlineVariant | neutralVariant | 80 | 30 |
| inverseSurface | neutral | 20 | 90 |
| inverseOnSurface | neutral | 95 | 20 |
| inversePrimary | primary | 80 | 40 |
| shadow | neutral | 0 | 0 |
| scrim | neutral | 0 | 0 |

Note: secondaryFixed, secondaryFixedDim, onSecondaryFixed, onSecondaryFixedVariant and their tertiary equivalents also follow the same pattern (90/80/10/30 for fixed variants).

### Factory Entry Point

```cpp
auto ColorScheme::fromSeed(Color seed, bool isDark, float contrastLevel) -> ColorScheme {
    auto sourceHct = detail::Hct::fromInt(seed);         // 1. Parse seed
    auto scheme = detail::DynamicScheme(sourceHct, isDark, contrastLevel);  // 2. Build 6 palettes
    ColorScheme result;
    // 3. Resolve each role
    result.primary = scheme.resolve(detail::MaterialDynamicColors::primary());
    result.onPrimary = scheme.resolve(detail::MaterialDynamicColors::onPrimary());
    // ... (all 30+ roles)
    return result;
}
```

## File Size Estimate

| Section | Lines |
|---------|-------|
| License / header guards / includes | ~20 |
| CAM16 (port from MCU) | ~250 |
| HCT (port from MCU) | ~150 |
| TonalPalette | ~100 |
| DynamicScheme | ~80 |
| MaterialDynamicColors (role table) | ~300 |
| ContrastCurve / ToneDelta / utils | ~100 |
| ColorScheme struct + fromSeed() | ~200 |
| **Total** | **~1200** |

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| HCT solver Newton iteration may not converge for extreme colors | Port MCU's `bisectToLimit` fallback + clamp to sRGB gamut |
| CAM16 math is complex and easy to mistranslate | Port directly from MCU TypeScript, line-by-line, with same variable names |
| Single header file gets very large | Use `detail` namespace to hide implementation; structure with clear section comments |
| Float/double precision differences from Dart/Java | Use `double` throughout; MCU uses double internally |

## References

- [material_color_utilities](https://github.com/material-foundation/material-color-utilities) — GitHub source
- [Flutter ColorScheme.fromSeed](https://api.flutter.dev/flutter/material/ColorScheme/ColorScheme.fromSeed.html)
- [M3 Color Roles](https://m3.material.io/styles/color/roles)
- [CAM16 color appearance model](https://en.wikipedia.org/wiki/CAM16)
