
'use strict';
(function () {
    'use strict';

    // 基础转换：镜像 ColorScheme.cpp 的 linearized/delinearized/xyz_from_rgb/lstar_from_y/y_from_lstar

    const K_EPSILON = 216 / 24389;
    const K_KAPPA = 24389 / 27;

    function linearized(value) {
        return value <= 0.04045 ? value / 12.92 : Math.pow((value + 0.055) / 1.055, 2.4);
    }

    function delinearized(channel) {
        const clamped = Math.min(1, Math.max(0, channel));
        const v = clamped <= 0.0031308 ? 12.92 * clamped : 1.055 * Math.pow(clamped, 1 / 2.4) - 0.055;
        return Math.round(Math.min(255, Math.max(0, v * 255)));
    }

    function xyzFromRgb(r, g, b) {
        const rl = linearized(r / 255) * 100;
        const gl = linearized(g / 255) * 100;
        const bl = linearized(b / 255) * 100;
        return {
            x: 0.41233895 * rl + 0.35762064 * gl + 0.18051042 * bl,
            y: 0.2126 * rl + 0.7152 * gl + 0.0722 * bl,
            z: 0.01932141 * rl + 0.11916382 * gl + 0.95034478 * bl,
        };
    }

    function lstarFromY(y) {
        const v = y / 100;
        return v <= K_EPSILON ? K_KAPPA * v : 116 * Math.cbrt(v) - 16;
    }

    function yFromLstar(lstar) {
        return lstar <= 8 ? 100 * lstar / K_KAPPA : 100 * Math.pow((lstar + 16) / 116, 3);
    }

    // CAM16 正向：镜像 ColorScheme.cpp 的 chromatic_adaptation/make_viewing_conditions/cam16_from_xyz。
    // VIEWING_CONDITIONS 属性名保留 C++ ViewingConditions 结构体字段名（rgbD/fl/n/z/nbb/ncb/aw/c/nc），保证 1:1 可追溯。

    function signedValue(value, magnitude) {
        return value < 0 ? -magnitude : magnitude;
    }

    function chromaticAdaptation(component, flFactor) {
        const p = Math.pow(flFactor * Math.abs(component) / 100, 0.42);
        return signedValue(component, 400 * p / (p + 27.13));
    }

    function makeViewingConditions() {
        const whiteX = 95.047;
        const whiteY = 100.0;
        const whiteZ = 108.883;

        const adaptingLuma = 200 / Math.PI * yFromLstar(50) / 100;
        const bgY = yFromLstar(50);

        const whiteR = 0.401288 * whiteX + 0.650173 * whiteY - 0.051461 * whiteZ;
        const whiteG = -0.250268 * whiteX + 1.204414 * whiteY + 0.045854 * whiteZ;
        const whiteB = -0.002079 * whiteX + 0.048952 * whiteY + 0.953127 * whiteZ;

        const discount = Math.min(1, Math.max(0, 1 - (1 / 3.6) * Math.exp((-adaptingLuma - 42) / 92)));

        const adaptK = 1 / (5 * adaptingLuma + 1);
        const kFourth = adaptK * adaptK * adaptK * adaptK;
        const flFactor = 0.2 * kFourth * (5 * adaptingLuma) + 0.1 * (1 - kFourth) * (1 - kFourth) * Math.cbrt(5 * adaptingLuma);

        const bgRatio = bgY / whiteY;
        const nbb = 0.725 / Math.pow(bgRatio, 0.2);

        const rgbD = [
            discount * (whiteY / whiteR) + 1 - discount,
            discount * (whiteY / whiteG) + 1 - discount,
            discount * (whiteY / whiteB) + 1 - discount,
        ];

        // aw 无 -0.305：对齐当前 C++（MCU 0.13.0 修正）
        const achromatic = (2 * chromaticAdaptation(rgbD[0] * whiteR, flFactor)
            + chromaticAdaptation(rgbD[1] * whiteG, flFactor)
            + 0.05 * chromaticAdaptation(rgbD[2] * whiteB, flFactor)) * nbb;

        return {
            rgbD,
            fl: flFactor,
            n: bgRatio,
            z: 1.48 + Math.sqrt(bgRatio),
            nbb,
            ncb: nbb,
            aw: achromatic,
            c: 0.69,
            nc: 1.0,
        };
    }

    const VIEWING_CONDITIONS = makeViewingConditions();

    function cam16FromXyz(xyz) {
        const vc = VIEWING_CONDITIONS;

        const lmsR = 0.401288 * xyz.x + 0.650173 * xyz.y - 0.051461 * xyz.z;
        const lmsG = -0.250268 * xyz.x + 1.204414 * xyz.y + 0.045854 * xyz.z;
        const lmsB = -0.002079 * xyz.x + 0.048952 * xyz.y + 0.953127 * xyz.z;

        const adaptR = chromaticAdaptation(vc.rgbD[0] * lmsR, vc.fl);
        const adaptG = chromaticAdaptation(vc.rgbD[1] * lmsG, vc.fl);
        const adaptB = chromaticAdaptation(vc.rgbD[2] * lmsB, vc.fl);

        const oppA = adaptR + (-12 * adaptG + adaptB) / 11;
        const oppB = (adaptR + adaptG - 2 * adaptB) / 9;

        let hue = Math.atan2(oppB, oppA) * 180 / Math.PI;
        if (hue < 0) {
            hue += 360;
        }

        const achromatic = (2 * adaptR + adaptG + 0.05 * adaptB) * vc.nbb;
        const j = 100 * Math.pow(achromatic / vc.aw, vc.c * vc.z);

        const hueRad = Math.atan2(oppB, oppA);
        const eccentricity = 0.25 * (Math.cos(hueRad + 2) + 3.8);
        const tFactor = 50000 / 13 * vc.nc * vc.ncb * eccentricity * Math.hypot(oppA, oppB) / (adaptR + adaptG + 1.05 * adaptB + 0.305);
        const alpha = Math.pow(tFactor, 0.9) * Math.pow(1.64 - Math.pow(0.29, vc.n), 0.73);

        return { hue, chroma: alpha * Math.sqrt(j / 100), j };
    }

    function hctFromColor(rgb) {
        const xyz = xyzFromRgb(rgb.r, rgb.g, rgb.b);
        const cam = cam16FromXyz(xyz);
        return { hue: cam.hue, chroma: cam.chroma, tone: lstarFromY(xyz.y) };
    }

    // HCT 逆向：镜像 ColorScheme.cpp 的 find_linear_rgb + 临界平面色相二分回退

    function inverseChromaticAdaptation(adapted) {
        const absA = Math.abs(adapted);
        const base = Math.max(0, 27.13 * absA / (400 - absA));
        return signedValue(adapted, 100 / VIEWING_CONDITIONS.fl * Math.pow(base, 1 / 0.42));
    }

    function findLinearRgb(hueDeg, chroma, y) {
        const vc = VIEWING_CONDITIONS;

        if (chroma < 1e-4) {
            return [y, y, y];
        }

        const hue = hueDeg * Math.PI / 180;
        const tInner = 1 / Math.pow(1.64 - Math.pow(0.29, vc.n), 0.73);
        const eHue = 0.25 * (Math.cos(hue + 2) + 3.8);
        const coefHue = eHue * (50000 / 13) * vc.nc * vc.ncb;
        const hSin = Math.sin(hue);
        const hCos = Math.cos(hue);

        let j = Math.sqrt(y) * 11;
        for (let round = 0; round < 5; round++) {
            const jNorm = j / 100;
            const alpha = j <= 0 ? 0 : chroma / Math.sqrt(jNorm);
            const tFactor = Math.pow(alpha * tInner, 1 / 0.9);
            const awScaled = vc.aw * Math.pow(jNorm, 1 / vc.c / vc.z);
            const achromNorm = awScaled / vc.nbb;
            const gamma = 23 * (achromNorm + 0.305) * tFactor / (23 * coefHue + 11 * tFactor * hCos + 108 * tFactor * hSin);
            const oppA = gamma * hCos;
            const oppB = gamma * hSin;
            const adaptR = (460 * achromNorm + 451 * oppA + 288 * oppB) / 1403;
            const adaptG = (460 * achromNorm - 891 * oppA - 261 * oppB) / 1403;
            const adaptB = (460 * achromNorm - 220 * oppA - 6300 * oppB) / 1403;

            const coneR = inverseChromaticAdaptation(adaptR) / vc.rgbD[0];
            const coneG = inverseChromaticAdaptation(adaptG) / vc.rgbD[1];
            const coneB = inverseChromaticAdaptation(adaptB) / vc.rgbD[2];

            const linX = 1.86206786 * coneR - 1.01125463 * coneG + 0.14918677 * coneB;
            const linY = 0.38752654 * coneR + 0.62144744 * coneG - 0.00897398 * coneB;
            const linZ = -0.0158415 * coneR - 0.03412294 * coneG + 1.04996444 * coneB;

            const linR = 3.2413775 * linX - 1.5376652 * linY - 0.4988538 * linZ;
            const linG = -0.9691453 * linX + 1.8758853 * linY + 0.0415659 * linZ;
            const linB = 0.0556209 * linX - 0.2039552 * linY + 1.0571799 * linZ;

            if (linR < 0 || linG < 0 || linB < 0) {
                return null;
            }
            const fnj = 0.2126 * linR + 0.7152 * linG + 0.0722 * linB;
            if (fnj <= 0) {
                return null;
            }
            if (round === 4 || Math.abs(fnj - y) < 0.002) {
                if (linR > 100.01 || linG > 100.01 || linB > 100.01) {
                    return null;
                }
                return [linR, linG, linB];
            }
            j -= (fnj - y) * j / (2 * fnj);
        }
        return null;
    }

    const SCALED_DISCOUNT_FROM_LINRGB = [
        [0.001200833568784504, 0.002389694492170889, 0.0002795742885861124],
        [0.0005891086651375999, 0.0029785502573438758, 0.0003270666104008398],
        [0.00010146692491640572, 0.0005364214359186694, 0.0032979401770712076],
    ];
    const Y_FROM_LINRGB = [0.2126, 0.7152, 0.0722];

    // 等价生成 Dart 的 _criticalPlanes（255 项），阈值取 0.040449936 对齐 MCU
    const CRITICAL_PLANES = (function () {
        const planes = new Array(255);
        for (let i = 0; i < 255; i++) {
            const norm = (i + 0.5) / 255;
            const lin = norm <= 0.040449936 ? norm / 12.92 : Math.pow((norm + 0.055) / 1.055, 2.4);
            planes[i] = 100 * lin;
        }
        return planes;
    })();

    function trueDelinearized(rgbComponent) {
        const normalized = rgbComponent / 100;
        return (normalized <= 0.0031308 ? normalized * 12.92 : 1.055 * Math.pow(normalized, 1 / 2.4) - 0.055) * 255;
    }

    function criticalPlaneBelow(value) {
        return Math.floor(value - 0.5);
    }

    function criticalPlaneAbove(value) {
        return Math.ceil(value - 0.5);
    }

    function sanitizeRadians(angle) {
        return (angle + 8 * Math.PI) % (2 * Math.PI);
    }

    function areInCyclicOrder(first, second, third) {
        return sanitizeRadians(second - first) < sanitizeRadians(third - first);
    }

    // 缩放折扣版自适应（无 fl 因子，fl 已折入矩阵）
    function scaledChromaticAdaptation(component) {
        const adapted = Math.pow(Math.abs(component), 0.42);
        return signedValue(component, 400 * adapted / (adapted + 27.13));
    }

    function hueOf(linrgb) {
        const scaledR = linrgb[0] * SCALED_DISCOUNT_FROM_LINRGB[0][0]
            + linrgb[1] * SCALED_DISCOUNT_FROM_LINRGB[0][1]
            + linrgb[2] * SCALED_DISCOUNT_FROM_LINRGB[0][2];
        const scaledG = linrgb[0] * SCALED_DISCOUNT_FROM_LINRGB[1][0]
            + linrgb[1] * SCALED_DISCOUNT_FROM_LINRGB[1][1]
            + linrgb[2] * SCALED_DISCOUNT_FROM_LINRGB[1][2];
        const scaledB = linrgb[0] * SCALED_DISCOUNT_FROM_LINRGB[2][0]
            + linrgb[1] * SCALED_DISCOUNT_FROM_LINRGB[2][1]
            + linrgb[2] * SCALED_DISCOUNT_FROM_LINRGB[2][2];

        const adaptR = scaledChromaticAdaptation(scaledR);
        const adaptG = scaledChromaticAdaptation(scaledG);
        const adaptB = scaledChromaticAdaptation(scaledB);

        const opponentA = (11 * adaptR - 12 * adaptG + adaptB) / 11;
        const opponentB = (adaptR + adaptG - 2 * adaptB) / 9;
        return Math.atan2(opponentB, opponentA);
    }

    function isBounded(value) {
        return 0 <= value && value <= 100;
    }

    const INVALID_VERTEX = [-1, -1, -1];

    function nthVertex(planeY, vertexIndex) {
        const coordA = vertexIndex % 4 <= 1 ? 0 : 100;
        const coordB = vertexIndex % 2 === 0 ? 0 : 100;

        if (vertexIndex < 4) {
            const green = coordA;
            const blue = coordB;
            const red = (planeY - green * Y_FROM_LINRGB[1] - blue * Y_FROM_LINRGB[2]) / Y_FROM_LINRGB[0];
            return isBounded(red) ? [red, green, blue] : INVALID_VERTEX;
        }
        if (vertexIndex < 8) {
            const blue = coordA;
            const red = coordB;
            const green = (planeY - red * Y_FROM_LINRGB[0] - blue * Y_FROM_LINRGB[2]) / Y_FROM_LINRGB[1];
            return isBounded(green) ? [red, green, blue] : INVALID_VERTEX;
        }
        const red = coordA;
        const green = coordB;
        const blue = (planeY - red * Y_FROM_LINRGB[0] - green * Y_FROM_LINRGB[1]) / Y_FROM_LINRGB[2];
        return isBounded(blue) ? [red, green, blue] : INVALID_VERTEX;
    }

    function intercept(source, mid, target) {
        return (mid - source) / (target - source);
    }

    function lerpPoint(source, factor, target) {
        return [
            source[0] + (target[0] - source[0]) * factor,
            source[1] + (target[1] - source[1]) * factor,
            source[2] + (target[2] - source[2]) * factor,
        ];
    }

    function setCoordinate(source, coordinate, target, axis) {
        return lerpPoint(source, intercept(source[axis], coordinate, target[axis]), target);
    }

    function midpoint(pointA, pointB) {
        return [
            (pointA[0] + pointB[0]) * 0.5,
            (pointA[1] + pointB[1]) * 0.5,
            (pointA[2] + pointB[2]) * 0.5,
        ];
    }

    function bisectToSegment(planeY, targetHue) {
        let left = INVALID_VERTEX;
        let right = INVALID_VERTEX;
        let leftHue = 0;
        let rightHue = 0;
        let initialized = false;
        let uncut = true;

        for (let vertexIndex = 0; vertexIndex < 12; vertexIndex++) {
            const mid = nthVertex(planeY, vertexIndex);
            if (mid[0] < 0) {
                continue;
            }
            const midHue = hueOf(mid);
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
        return [left, right];
    }

    function bisectToLimit(planeY, targetHue) {
        const segment = bisectToSegment(planeY, targetHue);
        let left = segment[0];
        let leftHue = hueOf(left);
        let right = segment[1];

        for (let axis = 0; axis < 3; axis++) {
            if (left[axis] === right[axis]) {
                continue;
            }
            let lPlane = -1;
            let rPlane = 255;
            if (left[axis] < right[axis]) {
                lPlane = criticalPlaneBelow(trueDelinearized(left[axis]));
                rPlane = criticalPlaneAbove(trueDelinearized(right[axis]));
            } else {
                lPlane = criticalPlaneAbove(trueDelinearized(left[axis]));
                rPlane = criticalPlaneBelow(trueDelinearized(right[axis]));
            }
            for (let i = 0; i < 8; i++) {
                if (Math.abs(rPlane - lPlane) <= 1) {
                    break;
                }
                const mPlane = Math.floor((lPlane + rPlane) * 0.5);
                const midPlaneCoordinate = CRITICAL_PLANES[mPlane];
                const mid = setCoordinate(left, midPlaneCoordinate, right, axis);
                const midHue = hueOf(mid);
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
        return midpoint(left, right);
    }

    function colorFromHct(hue, chroma, tone) {
        const clampedTone = Math.min(100, Math.max(0, tone));
        if (clampedTone <= 1e-4) {
            return { r: 0, g: 0, b: 0 };
        }
        if (clampedTone >= 99.9999) {
            return { r: 255, g: 255, b: 255 };
        }

        const y = yFromLstar(clampedTone);
        const normalizedHue = ((hue % 360) + 360) % 360;

        // 牛顿失败 → 临界平面色相二分（对齐 C++）
        const lin = findLinearRgb(normalizedHue, chroma, y) || bisectToLimit(y, normalizedHue * Math.PI / 180);

        return {
            r: delinearized(lin[0] / 100),
            g: delinearized(lin[1] / 100),
            b: delinearized(lin[2] / 100),
        };
    }

    // 调色板与配色方案：镜像 ColorScheme.cpp 的 make_palettes/light/dark

    const PRIMARY_CHROMA = 36;
    const SECONDARY_CHROMA = 16;
    const TERTIARY_CHROMA = 24;
    const TERTIARY_HUE_SHIFT = 60;
    const NEUTRAL_CHROMA = 6;
    const NEUTRAL_VARIANT_CHROMA = 8;
    const ERROR_HUE = 25;
    const ERROR_CHROMA = 84;

    function makePalettes(seedRgb) {
        const seedHct = hctFromColor(seedRgb);
        const tonalPalette = (hue, chroma) => ({
            hue,
            chroma,
            tone: (tone) => colorFromHct(hue, chroma, tone),
        });
        return {
            primary: tonalPalette(seedHct.hue, PRIMARY_CHROMA),
            secondary: tonalPalette(seedHct.hue, SECONDARY_CHROMA),
            tertiary: tonalPalette((seedHct.hue + TERTIARY_HUE_SHIFT) % 360, TERTIARY_CHROMA),
            neutral: tonalPalette(seedHct.hue, NEUTRAL_CHROMA),
            neutralVariant: tonalPalette(seedHct.hue, NEUTRAL_VARIANT_CHROMA),
            error: tonalPalette(ERROR_HUE, ERROR_CHROMA),
        };
    }

    const SCHEME_GROUPS = {
        primary: [
            'primary', 'onPrimary', 'primary_container', 'on_primary_container',
            'primary_fixed', 'primary_fixed_dim', 'on_primary_fixed', 'on_primary_fixed_variant',
        ],
        secondary: [
            'secondary', 'on_secondary', 'secondary_container', 'on_secondary_container',
            'secondary_fixed', 'secondary_fixed_dim', 'on_secondary_fixed', 'on_secondary_fixed_variant',
        ],
        tertiary: [
            'tertiary', 'on_tertiary', 'tertiary_container', 'on_tertiary_container',
            'tertiary_fixed', 'tertiary_fixed_dim', 'on_tertiary_fixed', 'on_tertiary_fixed_variant',
        ],
        error: ['error', 'on_error', 'error_container', 'on_error_container'],
        surface: [
            'surface', 'surface_dim', 'surface_bright', 'surface_container_lowest',
            'surface_container_low', 'surface_container', 'surface_container_high', 'surface_container_highest',
        ],
        other: [
            'on_surface', 'surface_variant', 'on_surface_variant', 'surface_tint',
            'outline', 'outline_variant', 'shadow', 'scrim',
            'inverse_surface', 'inverse_on_surface', 'inverse_primary',
        ],
    };

    const SCHEME_ORDER = Object.entries(SCHEME_GROUPS).flatMap(([group, keys]) => keys.map((key) => ({
        key,
        label: key.replace(/_([a-z])/g, (_, char) => char.toUpperCase()),
        group,
    })));

    // tone 值逐字段照抄 ColorScheme.cpp light()/dark()（浅色 on-container = 30）
    const TONE_SPEC = {
        light: {
            primary: ['primary', 40],
            onPrimary: ['primary', 100],
            primary_container: ['primary', 90],
            on_primary_container: ['primary', 30],
            primary_fixed: ['primary', 90],
            primary_fixed_dim: ['primary', 80],
            on_primary_fixed: ['primary', 10],
            on_primary_fixed_variant: ['primary', 30],
            secondary: ['secondary', 40],
            on_secondary: ['secondary', 100],
            secondary_container: ['secondary', 90],
            on_secondary_container: ['secondary', 30],
            secondary_fixed: ['secondary', 90],
            secondary_fixed_dim: ['secondary', 80],
            on_secondary_fixed: ['secondary', 10],
            on_secondary_fixed_variant: ['secondary', 30],
            tertiary: ['tertiary', 40],
            on_tertiary: ['tertiary', 100],
            tertiary_container: ['tertiary', 90],
            on_tertiary_container: ['tertiary', 30],
            tertiary_fixed: ['tertiary', 90],
            tertiary_fixed_dim: ['tertiary', 80],
            on_tertiary_fixed: ['tertiary', 10],
            on_tertiary_fixed_variant: ['tertiary', 30],
            error: ['error', 40],
            on_error: ['error', 100],
            error_container: ['error', 90],
            on_error_container: ['error', 30],
            surface: ['neutral', 98],
            surface_dim: ['neutral', 87],
            surface_bright: ['neutral', 98],
            surface_container_lowest: ['neutral', 100],
            surface_container_low: ['neutral', 96],
            surface_container: ['neutral', 94],
            surface_container_high: ['neutral', 92],
            surface_container_highest: ['neutral', 90],
            on_surface: ['neutral', 10],
            surface_variant: ['neutralVariant', 90],
            on_surface_variant: ['neutralVariant', 30],
            surface_tint: ['primary', 40],
            outline: ['neutralVariant', 50],
            outline_variant: ['neutralVariant', 80],
            shadow: ['neutral', 0],
            scrim: ['neutral', 0],
            inverse_surface: ['neutral', 20],
            inverse_on_surface: ['neutral', 95],
            inverse_primary: ['primary', 80],
        },
        dark: {
            primary: ['primary', 80],
            onPrimary: ['primary', 20],
            primary_container: ['primary', 30],
            on_primary_container: ['primary', 90],
            primary_fixed: ['primary', 90],
            primary_fixed_dim: ['primary', 80],
            on_primary_fixed: ['primary', 10],
            on_primary_fixed_variant: ['primary', 30],
            secondary: ['secondary', 80],
            on_secondary: ['secondary', 20],
            secondary_container: ['secondary', 30],
            on_secondary_container: ['secondary', 90],
            secondary_fixed: ['secondary', 90],
            secondary_fixed_dim: ['secondary', 80],
            on_secondary_fixed: ['secondary', 10],
            on_secondary_fixed_variant: ['secondary', 30],
            tertiary: ['tertiary', 80],
            on_tertiary: ['tertiary', 20],
            tertiary_container: ['tertiary', 30],
            on_tertiary_container: ['tertiary', 90],
            tertiary_fixed: ['tertiary', 90],
            tertiary_fixed_dim: ['tertiary', 80],
            on_tertiary_fixed: ['tertiary', 10],
            on_tertiary_fixed_variant: ['tertiary', 30],
            error: ['error', 80],
            on_error: ['error', 20],
            error_container: ['error', 30],
            on_error_container: ['error', 90],
            surface: ['neutral', 6],
            surface_dim: ['neutral', 6],
            surface_bright: ['neutral', 24],
            surface_container_lowest: ['neutral', 4],
            surface_container_low: ['neutral', 10],
            surface_container: ['neutral', 12],
            surface_container_high: ['neutral', 17],
            surface_container_highest: ['neutral', 22],
            on_surface: ['neutral', 90],
            surface_variant: ['neutralVariant', 30],
            on_surface_variant: ['neutralVariant', 80],
            surface_tint: ['primary', 80],
            outline: ['neutralVariant', 60],
            outline_variant: ['neutralVariant', 30],
            shadow: ['neutral', 0],
            scrim: ['neutral', 0],
            inverse_surface: ['neutral', 90],
            inverse_on_surface: ['neutral', 20],
            inverse_primary: ['primary', 40],
        },
    };

    function palettes(seedHex) {
        const seedRgb = hexToRgb(seedHex);
        if (seedRgb === null) {
            throw new Error('palettes: 非法 hex 输入: ' + seedHex);
        }
        return makePalettes(seedRgb);
    }

    function scheme(seedHex, brightness) {
        const spec = TONE_SPEC[brightness] || TONE_SPEC.light;
        const paletteSet = palettes(seedHex);
        const result = {};
        for (const [key, [paletteName, tone]] of Object.entries(spec)) {
            result[key] = rgbToHex(paletteSet[paletteName].tone(tone));
        }
        return result;
    }

    // 工具函数

    function hexToRgb(hex) {
        const match = /^#([0-9a-f]{3}|[0-9a-f]{6})$/i.exec(hex);
        if (match === null) {
            return null;
        }
        const digits = match[1];
        if (digits.length === 3) {
            return {
                r: parseInt(digits[0] + digits[0], 16),
                g: parseInt(digits[1] + digits[1], 16),
                b: parseInt(digits[2] + digits[2], 16),
            };
        }
        return {
            r: parseInt(digits.slice(0, 2), 16),
            g: parseInt(digits.slice(2, 4), 16),
            b: parseInt(digits.slice(4, 6), 16),
        };
    }

    function rgbToHex(rgb) {
        const channel = (value) => Math.min(255, Math.max(0, Math.round(value))).toString(16).padStart(2, '0');
        return '#' + channel(rgb.r) + channel(rgb.g) + channel(rgb.b);
    }

    async function copyText(text) {
        try {
            await navigator.clipboard.writeText(text);
            return true;
        } catch {
            const textarea = document.createElement('textarea');
            textarea.value = text;
            textarea.style.position = 'fixed';
            textarea.style.opacity = '0';
            document.body.appendChild(textarea);
            textarea.select();
            const ok = document.execCommand('copy');
            textarea.remove();
            return ok;
        }
    }

    function selfCheck() {
        const failures = [];
        const samples = ['#ff0000', '#00ff00', '#0000ff', '#ffffff', '#000000', '#808080', '#6750a4', '#ffc107'];
        for (const hex of samples) {
            const rgb = NekoHCT.hexToRgb(hex);
            const hct = NekoHCT.hctFromColor(rgb);
            const back = NekoHCT.colorFromHct(hct.hue, hct.chroma, hct.tone);
            const maxErr = Math.max(Math.abs(rgb.r - back.r), Math.abs(rgb.g - back.g), Math.abs(rgb.b - back.b));
            if (maxErr > 2) {
                failures.push(hex + ' round-trip err=' + maxErr);
            }
        }
        for (const brightness of ['light', 'dark']) {
            const scheme = NekoHCT.scheme('#6750a4', brightness);
            for (const { key } of NekoHCT.SCHEME_ORDER) {
                if (!/^#[0-9a-f]{6}$/.test(scheme[key])) {
                    failures.push(key + ' missing/invalid in ' + brightness);
                }
            }
        }
        if (failures.length === 0) {
            console.log('[selfCheck] PASS');
        } else {
            console.error('[selfCheck] FAIL', failures);
        }
        return { pass: failures.length === 0, failures };
    }

    const NekoHCT = {
        hexToRgb,
        rgbToHex,
        hctFromColor,
        colorFromHct,
        palettes,
        scheme,
        selfCheck,
        copyText,
        SCHEME_ORDER,
    };
    window.NekoHCT = NekoHCT;

    // 页面加载自检：失败时在标题给出可见标记（供人工验证兜底）
    const __sc = NekoHCT.selfCheck();
    if (!__sc.pass) {
        document.title = '⚠ selfCheck FAIL';
    }
})();

// 页面逻辑：主题（页面主题 + 角色表 brightness 联动）、Seed 选取、色带、角色表、导出

const { copyText } = NekoHCT;

function randomSeedHex() {
    return '#' + Math.floor(Math.random() * 0xffffff).toString(16).padStart(6, '0');
}

const state = { seed: randomSeedHex() };
let currentTheme = window.matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light';

function showToast(msg) {
    let toastEl = document.querySelector('.toast');
    if (toastEl === null) {
        toastEl = document.createElement('div');
        toastEl.className = 'toast';
        document.body.appendChild(toastEl);
    }
    toastEl.textContent = msg;
    toastEl.classList.add('visible');
    clearTimeout(toastEl._timer);
    toastEl._timer = setTimeout(() => toastEl.classList.remove('visible'), 1200);
}

// 页面级色值转换（RGB→HSL/HSV/CMYK、VEC4 浮点），仅供提示框显示，不进入 NekoHCT 引擎
function rgbToHsl(rgb) {
    const rn = rgb.r / 255;
    const gn = rgb.g / 255;
    const bn = rgb.b / 255;
    const max = Math.max(rn, gn, bn);
    const min = Math.min(rn, gn, bn);
    const l = (max + min) / 2;
    let h = 0;
    let s = 0;
    if (max !== min) {
        const d = max - min;
        s = l > 0.5 ? d / (2 - max - min) : d / (max + min);
        if (max === rn) {
            h = (gn - bn) / d + (gn < bn ? 6 : 0);
        } else if (max === gn) {
            h = (bn - rn) / d + 2;
        } else {
            h = (rn - gn) / d + 4;
        }
        h *= 60;
    }
    return { h, s: s * 100, l: l * 100 };
}

function rgbToHsv(rgb) {
    const rn = rgb.r / 255;
    const gn = rgb.g / 255;
    const bn = rgb.b / 255;
    const max = Math.max(rn, gn, bn);
    const min = Math.min(rn, gn, bn);
    const d = max - min;
    let h = 0;
    const s = max === 0 ? 0 : d / max;
    if (d !== 0) {
        if (max === rn) {
            h = (gn - bn) / d + (gn < bn ? 6 : 0);
        } else if (max === gn) {
            h = (bn - rn) / d + 2;
        } else {
            h = (rn - gn) / d + 4;
        }
        h *= 60;
    }
    return { h, s: s * 100, v: max * 100 };
}

function rgbToCmyk(rgb) {
    const rn = rgb.r / 255;
    const gn = rgb.g / 255;
    const bn = rgb.b / 255;
    const k = 1 - Math.max(rn, gn, bn);
    if (k === 1) {
        return { c: 0, m: 0, y: 0, k: 100 };
    }
    return {
        c: (1 - rn - k) / (1 - k) * 100,
        m: (1 - gn - k) / (1 - k) * 100,
        y: (1 - bn - k) / (1 - k) * 100,
        k: k * 100,
    };
}

// 各格式色值单行纯值（右键复制子菜单直接复制，不带名称前缀）
function colorValueLines(rgb) {
    const hsl = rgbToHsl(rgb);
    const hsv = rgbToHsv(rgb);
    const cmyk = rgbToCmyk(rgb);
    return {
        hex: NekoHCT.rgbToHex(rgb).toUpperCase(),
        rgb: rgb.r + ', ' + rgb.g + ', ' + rgb.b,
        hsl: Math.round(hsl.h) % 360 + '°, ' + Math.round(hsl.s) + '%, ' + Math.round(hsl.l) + '%',
        hsv: Math.round(hsv.h) % 360 + '°, ' + Math.round(hsv.s) + '%, ' + Math.round(hsv.v) + '%',
        cmyk: [cmyk.c, cmyk.m, cmyk.y, cmyk.k].map((value) => Math.round(value) + '%').join(', '),
        vec4: [rgb.r / 255, rgb.g / 255, rgb.b / 255, 1].map((value) => value.toFixed(3)).join(', '),
    };
}

function formatColorValues(rgb) {
    const lines = colorValueLines(rgb);
    return 'RGB: ' + lines.rgb + '\nHSL: ' + lines.hsl + '\nHSV: ' + lines.hsv + '\nCMYK: ' + lines.cmyk + '\nVEC4: ' + lines.vec4;
}

// 统一富提示框：标题区（名称 + 色值）+ 分隔线 + 内容区（用途说明 + 多行色值）；单例 fixed 定位，样式跟随主题 CSS 变量，右/下边缘防溢出
let tipRef = null;

function showTip(title, hex, body, x, y) {
    if (tipRef === null) {
        const el = document.createElement('div');
        el.className = 'tip';
        el.innerHTML = '<div class="tip-title"><span class="tip-name"></span><span class="tip-hex"></span></div>'
            + '<div class="tip-divider"></div><div class="tip-body"></div><div class="tip-values"></div>';
        document.body.appendChild(el);
        tipRef = {
            el,
            name: el.querySelector('.tip-name'),
            hex: el.querySelector('.tip-hex'),
            body: el.querySelector('.tip-body'),
            values: el.querySelector('.tip-values'),
        };
    }
    tipRef.name.textContent = title;
    tipRef.hex.textContent = hex;
    tipRef.body.textContent = body;
    const rgb = NekoHCT.hexToRgb(hex);
    tipRef.values.textContent = rgb === null ? '' : formatColorValues(rgb);
    tipRef.el.classList.add('visible');
    tipRef.el.style.left = Math.min(x + 12, window.innerWidth - tipRef.el.offsetWidth - 8) + 'px';
    tipRef.el.style.top = Math.min(y + 12, window.innerHeight - tipRef.el.offsetHeight - 8) + 'px';
}

function hideTip() {
    if (tipRef !== null) {
        tipRef.el.classList.remove('visible');
    }
}

// 页面主题色由当前 seed 的 ColorScheme 角色派生，seed 或主题变更时统一走此函数刷新 CSS 变量
function applyPageTheme(brightness, seedHex) {
    const scheme = NekoHCT.scheme(seedHex, brightness);
    const rootStyle = document.documentElement.style;
    rootStyle.setProperty('--bg', scheme.surface);
    rootStyle.setProperty('--panel', scheme.surface_container_lowest);
    rootStyle.setProperty('--panel-2', scheme.surface_container);
    rootStyle.setProperty('--text', scheme.on_surface);
    rootStyle.setProperty('--muted', scheme.on_surface_variant);
    rootStyle.setProperty('--accent', scheme.primary);
    rootStyle.setProperty('--on-accent', scheme.onPrimary);
    rootStyle.setProperty('--border', scheme.outline_variant);
}

function renderPaletteRamps(seedHex) {
    const palettes = NekoHCT.palettes(seedHex);
    for (const [name, pal] of Object.entries(palettes)) {
        const container = document.getElementById('ramp-' + name);
        container.textContent = '';
        for (let tone = 5; tone <= 95; tone += 5) {
            const rgb = pal.tone(tone);
            const hex = NekoHCT.rgbToHex(rgb);
            const hct = NekoHCT.hctFromColor(rgb);
            const tipBody = name + ' 色板 · H:' + hct.hue.toFixed(0) + ' C:' + hct.chroma.toFixed(1) + ' T:' + hct.tone.toFixed(0);
            const cell = document.createElement('div');
            cell.className = 'tone-cell';
            cell.style.background = hex;
            cell.dataset.tone = tone;
            cell.dataset.hex = hex;
            cell.addEventListener('click', () => copyText(hex).then(() => showToast('已复制 ' + hex)));
            cell.addEventListener('mouseenter', (e) => showTip('tone ' + tone, hex, tipBody, e.clientX, e.clientY));
            cell.addEventListener('mousemove', (e) => showTip('tone ' + tone, hex, tipBody, e.clientX, e.clientY));
            cell.addEventListener('mouseleave', hideTip);
            container.appendChild(cell);
        }
    }
}

// 常用颜色：Material Design 500 级 + 中性色，点击选作 seed（走 applySeed 统一管线），hover 显示名称
const COMMON_COLORS = [
    { name: '红色', hex: '#f44336' },
    { name: '粉色', hex: '#e91e63' },
    { name: '紫色', hex: '#9c27b0' },
    { name: '深紫', hex: '#673ab7' },
    { name: '靛蓝', hex: '#3f51b5' },
    { name: '蓝色', hex: '#2196f3' },
    { name: '浅蓝', hex: '#03a9f4' },
    { name: '青色', hex: '#00bcd4' },
    { name: '蓝绿', hex: '#009688' },
    { name: '绿色', hex: '#4caf50' },
    { name: '浅绿', hex: '#8bc34a' },
    { name: '黄绿', hex: '#cddc39' },
    { name: '黄色', hex: '#ffeb3b' },
    { name: '琥珀', hex: '#ffc107' },
    { name: '橙色', hex: '#ff9800' },
    { name: '深橙', hex: '#ff5722' },
    { name: '棕色', hex: '#795548' },
    { name: '灰色', hex: '#9e9e9e' },
    { name: '蓝灰', hex: '#607d8b' },
    { name: '黑色', hex: '#000000' },
    { name: '白色', hex: '#ffffff' },
];

function commonColorDesc(name) {
    if (name === '黑色' || name === '白色') {
        return name === '黑色' ? '中性色，纯黑' : '中性色，纯白';
    }
    return 'Material Design ' + name + ' 500';
}

function renderCommonColors() {
    const wrap = document.getElementById('common-colors');
    wrap.textContent = '';
    for (const { name, hex } of COMMON_COLORS) {
        const cell = document.createElement('button');
        cell.type = 'button';
        cell.className = 'common-color';
        cell.style.background = hex;
        cell.dataset.hex = hex;
        cell.addEventListener('click', () => applySeed(hex));
        const desc = commonColorDesc(name);
        cell.addEventListener('mouseenter', (e) => showTip(name, hex, desc, e.clientX, e.clientY));
        cell.addEventListener('mousemove', (e) => showTip(name, hex, desc, e.clientX, e.clientY));
        cell.addEventListener('mouseleave', hideTip);
        wrap.appendChild(cell);
    }
}

const seedColorInput = document.getElementById('seed-color');
const seedHexInput = document.getElementById('seed-hex');
const seedRandomButton = document.getElementById('seed-random');

// 流星雨背景装饰：参考实现风格——--x 横向铺开、--z 深度视差、--d 延迟错开，固定 -45° 同向划过；
// 每颗流星颜色从当前 scheme 的 47 个角色中随机选取（过滤深色角色保证两种主题下可见）；seed/主题变更时重新生成；reduced-motion 时禁用
const METEOR_COUNT = 15;
let meteorContainer = null;

function randomSchemeColor() {
    const scheme = NekoHCT.scheme(state.seed, currentTheme);
    // 排除纯黑系（shadow/scrim 等 tone < 25），保证流星在深/浅背景上都有对比
    const keys = Object.keys(scheme).filter((key) => NekoHCT.hctFromColor(NekoHCT.hexToRgb(scheme[key])).tone >= 25);
    return scheme[keys[Math.floor(Math.random() * keys.length)]];
}

function refreshMeteors() {
    if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) {
        return;
    }
    if (meteorContainer === null) {
        meteorContainer = document.createElement('div');
        meteorContainer.className = 'meteors';
        document.body.appendChild(meteorContainer);
    }
    meteorContainer.textContent = '';
    for (let i = 0; i < METEOR_COUNT; i++) {
        const meteor = document.createElement('div');
        meteor.className = 'meteor';
        // 参考实现：无 left/top 锚点（元素默认容器静态位置），全靠 --x 横向位移 + --z 纵深视差铺开
        meteor.style.setProperty('--x', (3 + Math.random() * 18).toFixed(1));
        meteor.style.setProperty('--z', (3 - Math.random() * 12).toFixed(1));
        meteor.style.setProperty('--d', (1 + Math.random() * 2).toFixed(1));
        meteor.style.setProperty('--mc', randomSchemeColor());
        meteorContainer.appendChild(meteor);
    }
}

// seed 变更需联动刷新页面主题、色带、角色表与导出预览，故各路输入统一走此管线
function applySeed(hex) {
    state.seed = hex;
    seedColorInput.value = hex;
    seedHexInput.value = hex;
    renderPaletteRamps(hex);
    renderRoleTable(currentTheme);
    cppExportPre.textContent = buildExport(hex, currentExportLang);
    applyPageTheme(currentTheme, hex);
    refreshMeteors();
    if (refPage.hidden === false) {
        renderRefControls();
    }
}

seedHexInput.addEventListener('change', () => {
    const rgb = NekoHCT.hexToRgb(seedHexInput.value.trim());
    if (rgb === null) {
        seedHexInput.value = state.seed;
        return;
    }
    applySeed(NekoHCT.rgbToHex(rgb));
});

seedRandomButton.addEventListener('click', () => applySeed(randomSeedHex()));

seedColorInput.addEventListener('input', () => applySeed(seedColorInput.value));

// 原生选色弹层确认时兜底刷新 seed（拖动预览由 input 事件承担，change 保证点确定后状态一致）
seedColorInput.addEventListener('change', () => applySeed(seedColorInput.value));

// 自定义右键菜单：fixed 定位单例（右/下边缘防溢出），点击外部/ESC/执行菜单项后关闭
const ctxMenu = document.getElementById('ctx-menu');
const ctxCopyColorItem = document.getElementById('ctx-copy-color');
const ctxSepCopy = document.getElementById('ctx-sep-copy');
const ctxCopySubmenu = document.getElementById('ctx-copy-submenu');
let ctxMenuX = 0;
let ctxMenuY = 0;
let ctxCopyHex = null;

function showCtxMenu(x, y) {
    ctxMenu.hidden = false;
    ctxMenu.style.left = Math.max(8, Math.min(x, window.innerWidth - ctxMenu.offsetWidth - 8)) + 'px';
    ctxMenu.style.top = Math.max(8, Math.min(y, window.innerHeight - ctxMenu.offsetHeight - 8)) + 'px';
    // 子菜单默认向右展开，贴近右缘时向左（估计子菜单宽度 250）
    ctxMenu.classList.toggle('open-left', x + ctxMenu.offsetWidth + 250 > window.innerWidth);
}

function hideCtxMenu() {
    ctxMenu.hidden = true;
}

document.addEventListener('contextmenu', (e) => {
    // 输入框内放行原生右键（hex 文本框粘贴/全选、取色器查看颜色值），其余位置拦截显示自定义菜单
    if (e.target.closest('input')) {
        return;
    }
    e.preventDefault();
    ctxMenuX = e.clientX;
    ctxMenuY = e.clientY;
    const colorCell = e.target.closest('.tone-cell, .role-cell, .common-color');
    // 主路径取渲染时绑定的 dataset.hex；回退按类型读取（role-cell 的 hex 文本），防渲染重构遗漏
    let hex = colorCell === null ? null : (colorCell.dataset.hex || null);
    if (hex === null && colorCell !== null) {
        const hexLabel = colorCell.querySelector('.role-cell-hex');
        if (hexLabel !== null) {
            hex = hexLabel.textContent;
        }
    }
    ctxCopyHex = hex;
    ctxCopyColorItem.hidden = ctxCopyHex === null;
    ctxSepCopy.hidden = ctxCopyHex === null;
    // 命中色块时按当前色值填充子菜单各格式预览
    if (ctxCopyHex !== null) {
        const rgb = NekoHCT.hexToRgb(ctxCopyHex);
        const lines = rgb === null ? null : colorValueLines(rgb);
        for (const item of ctxCopySubmenu.querySelectorAll('.ctx-item')) {
            item.querySelector('.ctx-val').textContent = lines === null ? '' : lines[item.dataset.format];
        }
    }
    showCtxMenu(e.clientX, e.clientY);
});

document.addEventListener('click', hideCtxMenu);

document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape') {
        hideCtxMenu();
    }
});

// 子菜单项委托：点击按所选格式复制右键时缓存的色块颜色
ctxCopySubmenu.addEventListener('click', (e) => {
    const btn = e.target.closest('.ctx-item');
    if (btn === null || ctxCopyHex === null) {
        return;
    }
    const text = colorValueLines(NekoHCT.hexToRgb(ctxCopyHex))[btn.dataset.format] || ctxCopyHex;
    copyText(text).then(() => showToast('已复制 ' + btn.textContent + ' ' + text));
    hideCtxMenu();
});

// 点击「复制颜色」根项也可展开/收起子菜单（hover 展开之外的可发现性兜底）；阻止冒泡避免 document click 误关菜单
ctxCopyColorItem.addEventListener('click', (e) => {
    e.stopPropagation();
    ctxCopySubmenu.classList.toggle('show');
});

// 选色器从右键位置弹出：将取色 input 固定定位到鼠标坐标（1px 透明）再 click()，原生弹层随元素位置出现
document.getElementById('ctx-picker').addEventListener('click', () => {
    hideCtxMenu();
    seedColorInput.style.position = 'fixed';
    seedColorInput.style.left = ctxMenuX + 'px';
    seedColorInput.style.top = ctxMenuY + 'px';
    seedColorInput.style.width = '1px';
    seedColorInput.style.height = '1px';
    seedColorInput.style.opacity = '0';
    seedColorInput.click();
    setTimeout(() => {
        seedColorInput.style.position = '';
        seedColorInput.style.left = '';
        seedColorInput.style.top = '';
        seedColorInput.style.width = '';
        seedColorInput.style.height = '';
        seedColorInput.style.opacity = '';
    }, 0);
});

document.getElementById('ctx-random').addEventListener('click', () => {
    hideCtxMenu();
    applySeed(randomSeedHex());
});

document.getElementById('ctx-copy-accent').addEventListener('click', () => {
    hideCtxMenu();
    const accent = getComputedStyle(document.documentElement).getPropertyValue('--accent').trim();
    copyText(accent).then(() => showToast('已复制主题色 ' + accent));
});

// 47 角色用途说明（Material Design 语义，与 SCHEME_ORDER 的 key 一一对应，提示框内容区数据源）
const ROLE_INFO = {
    // primary
    primary: '主色，用于主要组件与强调',
    onPrimary: '主色上的文字与图标',
    primary_container: '主色容器背景（卡片、侧栏等）',
    on_primary_container: '主色容器上的文字与图标',
    primary_fixed: '固定主色（浅色主题强调）',
    primary_fixed_dim: '固定主色的暗调变体',
    on_primary_fixed: '固定主色上的文字',
    on_primary_fixed_variant: '固定主色变体上的文字',
    // secondary
    secondary: '次色，用于次级组件与强调',
    on_secondary: '次色上的文字与图标',
    secondary_container: '次色容器背景',
    on_secondary_container: '次色容器上的文字与图标',
    secondary_fixed: '固定次色（浅色主题强调）',
    secondary_fixed_dim: '固定次色的暗调变体',
    on_secondary_fixed: '固定次色上的文字',
    on_secondary_fixed_variant: '固定次色变体上的文字',
    // tertiary
    tertiary: '第三色，用于品牌区分与个性化点缀',
    on_tertiary: '第三色上的文字与图标',
    tertiary_container: '第三色容器背景',
    on_tertiary_container: '第三色容器上的文字与图标',
    tertiary_fixed: '固定第三色（浅色主题强调）',
    tertiary_fixed_dim: '固定第三色的暗调变体',
    on_tertiary_fixed: '固定第三色上的文字',
    on_tertiary_fixed_variant: '固定第三色变体上的文字',
    // error
    error: '错误状态色，用于错误提示与校验反馈',
    on_error: '错误色上的文字与图标',
    error_container: '错误容器背景（错误消息区域）',
    on_error_container: '错误容器上的文字与图标',
    // surface
    surface: '页面背景',
    surface_dim: '降调表面（侧栏、面板等次要区域）',
    surface_bright: '高亮表面',
    surface_container_lowest: '表面容器最低层级（最贴近表面，用于层级分层）',
    surface_container_low: '表面容器低层级',
    surface_container: '表面容器中层级（默认表面容器）',
    surface_container_high: '表面容器高层级',
    surface_container_highest: '表面容器最高层级（层级对比最强）',
    // other
    on_surface: '表面上的主要文字与图标',
    surface_variant: '表面变体（工具条、输入框等辅助表面）',
    on_surface_variant: '表面变体上的文字与图标',
    surface_tint: '表面着色（顶层表面色调，如侧栏浮层）',
    outline: '描边（边框、分割线、分隔元素）',
    outline_variant: '弱化描边（低强调分隔元素）',
    shadow: '阴影色（元素投影）',
    scrim: '遮罩色（弹层、抽屉背后的遮罩）',
    inverse_surface: '反色表面（深色浮动层，如 Snackbar）',
    inverse_on_surface: '反色表面上的文字与图标',
    inverse_primary: '反色主色（浮动层上的主色强调）',
};

// 控件配色参考：全屏覆盖层，按当前 seed 的 scheme() 角色渲染常见控件配色并标注所用角色
const REF_CONTROLS = [
    {
        name: 'Button',
        roles: ['primary', 'onPrimary'],
        preview: (scheme) => {
            const el = document.createElement('button');
            el.type = 'button';
            el.className = 'ref-btn';
            el.style.background = scheme.primary;
            el.style.color = scheme.onPrimary;
            el.textContent = '按钮';
            return el;
        },
    },
    {
        name: 'TextField',
        roles: ['surface_container_highest', 'on_surface', 'outline'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-field';
            el.style.background = scheme.surface_container_highest;
            el.style.color = scheme.on_surface;
            el.style.borderColor = scheme.outline;
            el.textContent = '输入文本';
            return el;
        },
    },
    {
        name: 'Checkbox',
        roles: ['primary', 'onPrimary', 'outline'],
        preview: (scheme) => {
            const checked = document.createElement('span');
            checked.className = 'ref-check';
            checked.style.background = scheme.primary;
            checked.style.color = scheme.onPrimary;
            checked.style.borderColor = scheme.primary;
            checked.textContent = '✓';
            const unchecked = document.createElement('span');
            unchecked.className = 'ref-check';
            unchecked.style.borderColor = scheme.outline;
            const wrap = document.createElement('div');
            wrap.style.display = 'flex';
            wrap.style.gap = '8px';
            wrap.appendChild(checked);
            wrap.appendChild(unchecked);
            return wrap;
        },
    },
    {
        name: 'Radio',
        roles: ['primary', 'onPrimary', 'outline'],
        preview: (scheme) => {
            const sel = document.createElement('span');
            sel.className = 'ref-radio';
            sel.style.borderColor = scheme.primary;
            const dot = document.createElement('span');
            dot.className = 'ref-radio-dot';
            dot.style.background = scheme.primary;
            sel.appendChild(dot);
            const unsel = document.createElement('span');
            unsel.className = 'ref-radio';
            unsel.style.borderColor = scheme.outline;
            const wrap = document.createElement('div');
            wrap.style.display = 'flex';
            wrap.style.gap = '8px';
            wrap.appendChild(sel);
            wrap.appendChild(unsel);
            return wrap;
        },
    },
    {
        name: 'Switch',
        roles: ['primary', 'onPrimary', 'outline'],
        preview: (scheme) => {
            const on = document.createElement('span');
            on.className = 'ref-switch';
            on.style.background = scheme.primary;
            const onThumb = document.createElement('span');
            onThumb.className = 'ref-switch-thumb';
            onThumb.style.background = scheme.onPrimary;
            on.appendChild(onThumb);
            const off = document.createElement('span');
            off.className = 'ref-switch';
            off.style.background = scheme.outline;
            const offThumb = document.createElement('span');
            offThumb.className = 'ref-switch-thumb';
            offThumb.style.background = scheme.surface_container_lowest;
            off.appendChild(offThumb);
            const wrap = document.createElement('div');
            wrap.style.display = 'flex';
            wrap.style.gap = '8px';
            wrap.appendChild(on);
            wrap.appendChild(off);
            return wrap;
        },
    },
    {
        name: 'Slider',
        roles: ['primary', 'surface_container_highest'],
        preview: (scheme) => {
            const track = document.createElement('div');
            track.className = 'ref-slider';
            track.style.background = scheme.surface_container_highest;
            const thumb = document.createElement('div');
            thumb.className = 'ref-slider-thumb';
            thumb.style.background = scheme.primary;
            track.appendChild(thumb);
            return track;
        },
    },
    {
        name: 'Card',
        roles: ['surface_container_lowest', 'on_surface', 'outline_variant'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-card';
            el.style.background = scheme.surface_container_lowest;
            el.style.color = scheme.on_surface;
            el.style.borderColor = scheme.outline_variant;
            el.textContent = '卡片内容';
            return el;
        },
    },
    {
        name: 'Dialog',
        roles: ['surface_container_high', 'on_surface'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-dialog';
            el.style.background = scheme.surface_container_high;
            el.style.color = scheme.on_surface;
            el.innerHTML = '<strong>对话框</strong><div>对话框内容区域</div>';
            return el;
        },
    },
    {
        name: 'SnackBar',
        roles: ['inverse_surface', 'inverse_on_surface'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-snackbar';
            el.style.background = scheme.inverse_surface;
            el.style.color = scheme.inverse_on_surface;
            el.textContent = '提示消息';
            return el;
        },
    },
    {
        name: 'Chip',
        roles: ['secondary_container', 'on_secondary_container'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-chip';
            el.style.background = scheme.secondary_container;
            el.style.color = scheme.on_secondary_container;
            el.textContent = '标签';
            return el;
        },
    },
    {
        name: 'TopAppBar',
        roles: ['surface', 'on_surface', 'outline_variant'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-appbar';
            el.style.background = scheme.surface;
            el.style.color = scheme.on_surface;
            el.style.borderColor = scheme.outline_variant;
            el.textContent = '顶栏标题';
            return el;
        },
    },
    {
        name: 'NavigationBar',
        roles: ['surface_container', 'primary', 'on_surface_variant'],
        preview: (scheme) => {
            const wrap = document.createElement('div');
            wrap.className = 'ref-navbar';
            wrap.style.background = scheme.surface_container;
            const labels = ['首页', '项目', '设置'];
            for (let i = 0; i < labels.length; i++) {
                const item = document.createElement('span');
                item.className = 'ref-nav-item';
                item.style.color = i === 0 ? scheme.primary : scheme.on_surface_variant;
                item.textContent = labels[i];
                wrap.appendChild(item);
            }
            return wrap;
        },
    },
    {
        name: 'List',
        roles: ['surface_container_lowest', 'on_surface', 'on_surface_variant', 'outline_variant'],
        preview: (scheme) => {
            const wrap = document.createElement('div');
            wrap.className = 'ref-list';
            const items = [
                { title: '列表项一', sub: '次要说明文字' },
                { title: '列表项二', sub: '次要说明文字' },
            ];
            for (const item of items) {
                const row = document.createElement('div');
                row.className = 'ref-list-item';
                row.style.borderColor = scheme.outline_variant;
                const title = document.createElement('span');
                title.className = 'ref-list-title';
                title.style.color = scheme.on_surface;
                title.textContent = item.title;
                const sub = document.createElement('span');
                sub.className = 'ref-list-sub';
                sub.style.color = scheme.on_surface_variant;
                sub.textContent = item.sub;
                row.appendChild(title);
                row.appendChild(sub);
                wrap.appendChild(row);
            }
            wrap.style.background = scheme.surface_container_lowest;
            return wrap;
        },
    },
    {
        name: 'Tooltip',
        roles: ['inverse_surface', 'inverse_on_surface'],
        preview: (scheme) => {
            const el = document.createElement('div');
            el.className = 'ref-tooltip';
            el.style.background = scheme.inverse_surface;
            el.style.color = scheme.inverse_on_surface;
            el.textContent = '提示文字';
            return el;
        },
    },
    {
        name: 'ProgressBar',
        roles: ['primary', 'surface_container_highest'],
        preview: (scheme) => {
            const track = document.createElement('div');
            track.className = 'ref-progress';
            track.style.background = scheme.surface_container_highest;
            const fill = document.createElement('div');
            fill.className = 'ref-progress-fill';
            fill.style.background = scheme.primary;
            track.appendChild(fill);
            return track;
        },
    },
];

// 页面切换：取色工具 / 控件配色（tab 组驱动，.page 容器显隐）
const refPage = document.getElementById('page-ref');

function renderRefControls() {
    const scheme = NekoHCT.scheme(state.seed, currentTheme);
    const body = document.getElementById('ref-body');
    body.textContent = '';
    for (const ctrl of REF_CONTROLS) {
        const item = document.createElement('div');
        item.className = 'ref-item';
        const name = document.createElement('div');
        name.className = 'ref-item-name';
        name.textContent = ctrl.name;
        const preview = document.createElement('div');
        preview.className = 'ref-preview';
        preview.appendChild(ctrl.preview(scheme));
        const roles = document.createElement('div');
        roles.className = 'ref-item-roles';
        roles.textContent = ctrl.roles.map((key) => key + ' ' + scheme[key]).join(' · ');
        item.appendChild(name);
        item.appendChild(preview);
        item.appendChild(roles);
        body.appendChild(item);
    }
}

function switchPage(pageId) {
    document.getElementById('page-picker').hidden = pageId !== 'picker';
    refPage.hidden = pageId !== 'ref';
    for (const tab of document.querySelectorAll('.page-tab')) {
        tab.setAttribute('aria-selected', String(tab.dataset.page === pageId));
    }
    document.getElementById('page-tabs').setAttribute('data-page', pageId);
    if (pageId === 'ref') {
        renderRefControls();
    }
}

for (const tab of document.querySelectorAll('.page-tab')) {
    tab.addEventListener('click', () => switchPage(tab.dataset.page));
}

// 角色表：每行 = 色块，label 与 hex 上下居中，文字色按色块 tone 选深/浅
function renderRoleTable(brightness) {
    const scheme = NekoHCT.scheme(state.seed, brightness);
    const wrap = document.getElementById('role-table');
    wrap.textContent = '';
    let groupTitle = null;
    let groupGrid = null;
    for (const { key, label, group } of NekoHCT.SCHEME_ORDER) {
        if (group !== groupTitle) {
            const heading = document.createElement('h4');
            heading.className = 'group-title';
            heading.textContent = group;
            groupGrid = document.createElement('div');
            groupGrid.className = 'role-grid';
            wrap.appendChild(heading);
            wrap.appendChild(groupGrid);
            groupTitle = group;
        }
        const hex = scheme[key];
        const tone = NekoHCT.hctFromColor(NekoHCT.hexToRgb(hex)).tone;
        const cell = document.createElement('button');
        cell.type = 'button';
        cell.className = 'role-cell';
        cell.style.background = hex;
        cell.style.color = tone > 50 ? '#111111' : '#ffffff';
        cell.dataset.hex = hex;
        cell.innerHTML = '<span class="role-cell-label">' + label + '</span><span class="role-cell-hex">' + hex + '</span>';
        const roleInfo = ROLE_INFO[key] || 'Material Design 颜色角色';
        cell.addEventListener('click', () => copyText(hex).then(() => showToast('已复制 ' + hex)));
        cell.addEventListener('mouseenter', (e) => showTip(label, hex, roleInfo, e.clientX, e.clientY));
        cell.addEventListener('mousemove', (e) => showTip(label, hex, roleInfo, e.clientX, e.clientY));
        cell.addEventListener('mouseleave', hideTip);
        groupGrid.appendChild(cell);
    }
}

const themeLightButton = document.getElementById('theme-light');
const themeDarkButton = document.getElementById('theme-dark');
const copyAllButton = document.getElementById('copy-all');

// 主题与角色表 brightness 联动：同一开关同时驱动 data-theme 属性、页面 CSS 变量、滑块位置与角色表数据
function setTheme(brightness) {
    currentTheme = brightness;
    themeLightButton.setAttribute('aria-pressed', String(brightness === 'light'));
    themeDarkButton.setAttribute('aria-pressed', String(brightness === 'dark'));
    document.getElementById('theme-toggle').setAttribute('data-theme', brightness);
    document.documentElement.dataset.theme = brightness;
    applyPageTheme(brightness, state.seed);
    renderRoleTable(brightness);
    refreshMeteors();
    if (refPage.hidden === false) {
        renderRefControls();
    }
}

themeLightButton.addEventListener('click', () => setTheme('light'));
themeDarkButton.addEventListener('click', () => setTheme('dark'));

copyAllButton.addEventListener('click', () => {
    const scheme = NekoHCT.scheme(state.seed, currentTheme);
    const text = NekoHCT.SCHEME_ORDER.map(({ key }) => key + ': ' + scheme[key]).join('\n');
    copyText(text).then(() => showToast('已复制全部 ' + NekoHCT.SCHEME_ORDER.length + ' 角色'));
});

// 导出 ColorScheme 角色字典（14 种语言，light/dark 两段，key 逐字匹配 ColorScheme.hpp，值 0xAARRGGBB / CSS 用 #RRGGBB）

function hexToArgb(hex) {
    const rgb = NekoHCT.hexToRgb(hex);
    const channel = (value) => value.toString(16).padStart(2, '0').toUpperCase();
    return '0xFF' + channel(rgb.r) + channel(rgb.g) + channel(rgb.b);
}

// 共享中间结构：{ light: [{key, hex}...], dark: [...] }，14 个语言生成器消费同一数据
function schemeData(seedHex) {
    const entries = (brightness) => {
        const scheme = NekoHCT.scheme(seedHex, brightness);
        return NekoHCT.SCHEME_ORDER.map(({ key }) => ({ key, hex: scheme[key] }));
    };
    return { light: entries('light'), dark: entries('dark') };
}

const EXPORT_LANGUAGES = [
    {
        id: 'cpp',
        name: 'C++',
        generate: (data) => {
            const lines = [];
            for (const brightness of ['light', 'dark']) {
                const mapName = brightness === 'light' ? 'kSchemeLight' : 'kSchemeDark';
                lines.push('// ' + brightness + '(seed) 角色字典（名称 → 0xAARRGGBB，字段名逐字匹配 ColorScheme.hpp）');
                lines.push('static const std::unordered_map<std::string, uint32_t> ' + mapName + ' = {');
                for (const { key, hex } of data[brightness]) {
                    lines.push('    {"' + key + '", ' + hexToArgb(hex) + '},');
                }
                lines.push('};');
                lines.push('');
            }
            return lines.join('\n');
        },
    },
    {
        id: 'json',
        name: 'JSON',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '        "' + key + '": ' + hexToArgb(hex) + ',').join('\n');
            return '{\n'
                + '    "light": {\n' + block(data.light) + '\n    },\n'
                + '    "dark": {\n' + block(data.dark) + '\n    }\n'
                + '}';
        },
    },
    {
        id: 'css',
        name: 'CSS 变量',
        generate: (data) => {
            const lines = [];
            for (const brightness of ['light', 'dark']) {
                lines.push('/* ' + brightness + '(seed) 角色变量（#RRGGBB） */');
                lines.push(":root[data-theme='" + brightness + "'] {");
                for (const { key, hex } of data[brightness]) {
                    lines.push('    --' + key + ': ' + hex + ';');
                }
                lines.push('}');
                lines.push('');
            }
            return lines.join('\n');
        },
    },
    {
        id: 'python',
        name: 'Python',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    "' + key + '": ' + hexToArgb(hex) + ',').join('\n');
            return 'LIGHT_SCHEME = {\n' + block(data.light) + '\n}\n\nDARK_SCHEME = {\n' + block(data.dark) + '\n}';
        },
    },
    {
        id: 'javascript',
        name: 'JavaScript',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    ' + key + ': ' + hexToArgb(hex) + ',').join('\n');
            return 'const kSchemeLight = {\n' + block(data.light) + '\n};\n\nconst kSchemeDark = {\n' + block(data.dark) + '\n};';
        },
    },
    {
        id: 'rust',
        name: 'Rust',
        generate: (data) => {
            const len = data.light.length;
            const block = (entries) => entries.map(({ key, hex }) => '    ("' + key + '", ' + hexToArgb(hex) + '),').join('\n');
            return 'pub const LIGHT_SCHEME: [(&str, u32); ' + len + '] = [\n' + block(data.light) + '\n];\n\n'
                + 'pub const DARK_SCHEME: [(&str, u32); ' + len + '] = [\n' + block(data.dark) + '\n];';
        },
    },
    {
        id: 'go',
        name: 'Go',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    "' + key + '": ' + hexToArgb(hex) + ',').join('\n');
            return 'var lightScheme = map[string]uint32{\n' + block(data.light) + '\n}\n\nvar darkScheme = map[string]uint32{\n' + block(data.dark) + '\n}';
        },
    },
    {
        id: 'java',
        name: 'Java',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '        put("' + key + '", ' + hexToArgb(hex) + ');').join('\n');
            return 'Map<String, Integer> lightScheme = new HashMap<>() {{\n' + block(data.light) + '\n    }};\n\n'
                + 'Map<String, Integer> darkScheme = new HashMap<>() {{\n' + block(data.dark) + '\n    }};';
        },
    },
    {
        id: 'dart',
        name: 'Dart',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => "    '" + key + "': " + hexToArgb(hex) + ',').join('\n');
            return 'const lightScheme = <String, int>{\n' + block(data.light) + '\n};\n\nconst darkScheme = <String, int>{\n' + block(data.dark) + '\n};';
        },
    },
    {
        id: 'csharp',
        name: 'C#',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    ["' + key + '"] = ' + hexToArgb(hex) + ',').join('\n');
            return 'var lightScheme = new Dictionary<string, uint> {\n' + block(data.light) + '\n};\n\nvar darkScheme = new Dictionary<string, uint> {\n' + block(data.dark) + '\n};';
        },
    },
    {
        id: 'php',
        name: 'PHP',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => "    '" + key + "' => " + hexToArgb(hex) + ',').join('\n');
            return '$lightScheme = [\n' + block(data.light) + '\n];\n\n$darkScheme = [\n' + block(data.dark) + '\n];';
        },
    },
    {
        id: 'kotlin',
        name: 'Kotlin',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    "' + key + '" to ' + hexToArgb(hex) + ',').join('\n');
            return 'val lightScheme = mapOf(\n' + block(data.light) + '\n)\n\nval darkScheme = mapOf(\n' + block(data.dark) + '\n)';
        },
    },
    {
        id: 'typescript',
        name: 'TypeScript',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    ' + key + ': ' + hexToArgb(hex) + ',').join('\n');
            return 'export const lightScheme: Record<string, number> = {\n' + block(data.light) + '\n};\n\nexport const darkScheme: Record<string, number> = {\n' + block(data.dark) + '\n};';
        },
    },
    {
        id: 'swift',
        name: 'Swift',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    "' + key + '": ' + hexToArgb(hex) + ',').join('\n');
            return 'let lightScheme: [String: UInt32] = [\n' + block(data.light) + '\n]\n\nlet darkScheme: [String: UInt32] = [\n' + block(data.dark) + '\n]';
        },
    },
    {
        id: 'toml',
        name: 'TOML',
        generate: (data) => {
            const lines = [];
            for (const brightness of ['light', 'dark']) {
                lines.push('# ' + brightness + '(seed) 角色字典');
                lines.push('[' + brightness + ']');
                for (const { key, hex } of data[brightness]) {
                    lines.push(key + ' = ' + hexToArgb(hex));
                }
                lines.push('');
            }
            return lines.join('\n');
        },
    },
    {
        id: 'yaml',
        name: 'YAML',
        generate: (data) => {
            const lines = [];
            for (const brightness of ['light', 'dark']) {
                lines.push('# ' + brightness + '(seed) 角色字典');
                lines.push(brightness + ':');
                for (const { key, hex } of data[brightness]) {
                    lines.push('  ' + key + ': ' + hexToArgb(hex));
                }
            }
            return lines.join('\n');
        },
    },
    {
        id: 'xaml',
        name: 'XAML',
        generate: (data) => {
            const block = (entries) => entries.map(({ key, hex }) => '    <Color x:Key="' + key + '">#' + hexToArgb(hex).slice(2) + '</Color>').join('\n');
            return '<!-- light(seed) 角色字典 -->\n<ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">\n'
                + block(data.light) + '\n</ResourceDictionary>\n\n'
                + '<!-- dark(seed) 角色字典 -->\n<ResourceDictionary xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation">\n'
                + block(data.dark) + '\n</ResourceDictionary>';
        },
    },
];

function buildExport(seedHex, language) {
    const generator = EXPORT_LANGUAGES.find((lang) => lang.id === language) || EXPORT_LANGUAGES[0];
    return generator.generate(schemeData(seedHex));
}

const cppExportPre = document.getElementById('cpp-export');
const cppCopyButton = document.getElementById('cpp-copy');
const exportLangSelect = document.getElementById('export-lang');
let currentExportLang = 'cpp';

function refreshExport() {
    cppExportPre.textContent = buildExport(state.seed, currentExportLang);
}

exportLangSelect.addEventListener('change', () => {
    currentExportLang = exportLangSelect.value;
    refreshExport();
});

cppCopyButton.addEventListener('click', () => {
    refreshExport();
    const langName = EXPORT_LANGUAGES.find((lang) => lang.id === currentExportLang).name;
    copyText(cppExportPre.textContent).then(() => showToast('已复制 ' + langName + ' 代码'));
});

// 初始化：随机 seed 同步取色器/hex 输入框，setTheme 统一按钮高亮、data-theme、页面主题变量与角色表数据
renderCommonColors();
seedColorInput.value = state.seed;
seedHexInput.value = state.seed;
renderPaletteRamps(state.seed);
setTheme(currentTheme);
refreshExport();
refreshMeteors();
