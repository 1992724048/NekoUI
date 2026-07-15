#pragma once

namespace neko::type {
    template<typename T, size_t N>
    struct Vec;

    template<typename T>
    struct Vec<T, 2> {
        T x, y;

        constexpr auto operator-(const Vec& other) const -> Vec {
            return {.x = x - other.x, .y = y - other.y};
        }

        constexpr auto operator==(const Vec&) const -> bool = default;
    };

    template<typename T>
    struct Vec<T, 3> {
        union {
            struct {
                T x, y, z;
            };

            struct {
                T r, g, b;
            };
        };

        constexpr auto operator==(const Vec&) const -> bool = default;
    };

    template<typename T>
    struct Vec<T, 4> {
        union {
            struct {
                T x, y, z, w;
            };

            struct {
                T r, g, b, a;
            };
        };

        constexpr auto operator==(const Vec&) const -> bool = default;
    };

    using Vec2 = Vec<float, 2>;
    using Vec3 = Vec<float, 3>;
    using Vec4 = Vec<float, 4>;
    using IVec2 = Vec<int, 2>;
    using IVec3 = Vec<int, 3>;
    using IVec4 = Vec<int, 4>;

    using Color = Vec4;
} // namespace neko::type
