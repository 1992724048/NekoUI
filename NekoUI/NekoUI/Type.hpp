#pragma once

#include <compare>
#include <cstdint>

namespace neko::type {
    template<typename T, size_t N>
    struct Vec;

    template<typename T>
    struct Vec<T, 2> {
        union {
            T x, width;
        };

        union {
            T y, height;
        };

        constexpr auto operator-(const Vec& other) const -> Vec {
            return {.x = x - other.x, .y = y - other.y};
        }

        constexpr auto operator==(const Vec& other) const -> bool {
            return x == other.x && y == other.y;
        }

        constexpr auto operator<=>(const Vec& other) const {
            if (auto c = x <=> other.x; c != 0) {
                return c;
            }
            return y <=> other.y;
        }
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

        constexpr auto operator==(const Vec& other) const -> bool {
            return x == other.x && y == other.y && z == other.z;
        }

        constexpr auto operator<=>(const Vec& other) const {
            if (auto c = x <=> other.x; c != 0) {
                return c;
            }
            if (auto c = y <=> other.y; c != 0) {
                return c;
            }
            return z <=> other.z;
        }
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

        constexpr auto operator==(const Vec& other) const -> bool {
            return x == other.x && y == other.y && z == other.z && w == other.w;
        }

        constexpr auto operator<=>(const Vec& other) const {
            if (auto c = x <=> other.x; c != 0) {
                return c;
            }
            if (auto c = y <=> other.y; c != 0) {
                return c;
            }
            if (auto c = z <=> other.z; c != 0) {
                return c;
            }
            return w <=> other.w;
        }
    };

    using Vec2 = Vec<float, 2>;
    using Vec3 = Vec<float, 3>;
    using Vec4 = Vec<float, 4>;
    using Vec2I = Vec<int, 2>;
    using Vec3I = Vec<int, 3>;
    using Vec4I = Vec<int, 4>;

    using Size = Vec2I;
    using Rect = Vec4I;

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
