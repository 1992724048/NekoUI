#pragma once

#include <cstdint>

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
                T x, y;

                union {
                    T z, width;
                };

                union {
                    T w, height;
                };
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
    using Vec2I = Vec<int, 2>;
    using Vec3I = Vec<int, 3>;
    using Vec4I = Vec<int, 4>;

    using Handle = void*;

    struct Color {
        uint32_t value;

        [[nodiscard]] constexpr auto r() const -> uint8_t {
            return static_cast<uint8_t>(value >> 24);
        }

        [[nodiscard]] constexpr auto g() const -> uint8_t {
            return static_cast<uint8_t>(value >> 16);
        }

        [[nodiscard]] constexpr auto b() const -> uint8_t {
            return static_cast<uint8_t>(value >> 8);
        }

        [[nodiscard]] constexpr auto a() const -> uint8_t {
            return static_cast<uint8_t>(value);
        }
    };
} // namespace neko::type
