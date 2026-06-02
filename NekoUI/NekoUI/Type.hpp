#pragma once
namespace neko::type {
    using Handle = void*;

    struct Color {
        std::uint8_t r;
        std::uint8_t g;
        std::uint8_t b;
        std::uint8_t a;
    };

    template<typename T>
    struct Vec2 {
        T x;
        T y;

        auto operator==(const Vec2& vec2) const -> bool {
            return vec2.x == x && vec2.y == y;
        }
    };

    template<typename T>
    struct Vec3 {
        T x;
        T y;
        T z;

        auto operator==(const Vec3& vec3) const -> bool {
            return vec3.x == x && vec3.y == y && vec3.z == z;
        }
    };

    template<typename T>
    struct Vec4 {
        T x;
        T y;
        T z;
        T w;

        auto operator==(const Vec4& vec4) const -> bool {
            return vec4.x == x && vec4.y == y && vec4.z == z && vec4.w == w;
        }
    };
} // namespace neko::type
