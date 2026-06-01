#pragma once
namespace neko::type {
    using Handle = void*;

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
    };
} // namespace neko::type
