#pragma once

namespace neko::type {
    struct Vec2 {
        float x, y;
    };

    struct Vec3 {
        float x, y, z;
    };

    struct Vec4 {
        union {
            struct {
                float x, y, z, w;
            };

            struct {
                float r, g, b, a;
            };
        };
    };

    struct IVec2 {
        int x, y;

        constexpr auto operator-(const IVec2& other) const -> IVec2 {
            return {.x = x - other.x, .y = y - other.y};
        }

        constexpr auto operator==(const IVec2&) const -> bool = default;
    };

    struct IVec3 {
        int x, y, z;
    };

    struct IVec4 {
        int x, y, z, w;
        constexpr auto operator==(const IVec4&) const -> bool = default;
    };

    using Color = Vec4;
} // namespace neko::type
