#pragma once

/*
 * 向量
 *
 * 右手系、Y 轴向上、相机朝 -Z
 *
 */

#include <cmath>

namespace taixu::core
{
    /**
     * 2D向量
     */
    struct Vec2
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    /**
     * 3D向量
     */
    struct Vec3
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
    };

    /**
     * 4D 向量
     */
    struct Vec4
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 0.0f;
    };

    // Vec2 utils

    constexpr Vec2 operator+(const Vec2 a, Vec2 b) noexcept { return {a.x + b.x, a.y + b.y}; }
    constexpr Vec2 operator-(Vec2 a, Vec2 b) noexcept { return {a.x - b.x, a.y - b.y}; }
    constexpr Vec2 operator-(Vec2 a) noexcept { return {-a.x, -a.y}; }
    constexpr Vec2 operator*(Vec2 a, float s) noexcept { return {a.x * s, a.y * s}; }
    constexpr Vec2 operator*(float s, Vec2 a) noexcept { return a * s; }
    constexpr Vec2 operator/(Vec2 a, float s) noexcept { return {a.x / s, a.y / s}; }
    constexpr Vec2& operator+=(Vec2& a, Vec2 b) noexcept { return a = a + b; }
    constexpr Vec2& operator-=(Vec2& a, Vec2 b) noexcept { return a = a - b; }

    constexpr float dot(Vec2 a, Vec2 b) noexcept { return a.x * b.x + a.y * b.y; }
    constexpr float length_squared(Vec2 a) noexcept { return dot(a, a); }

    // Vec3 utils

    constexpr Vec3 operator+(Vec3 a, Vec3 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
    constexpr Vec3 operator-(Vec3 a, Vec3 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
    constexpr Vec3 operator-(Vec3 a) noexcept { return {-a.x, -a.y, -a.z}; }
    constexpr Vec3 operator*(Vec3 a, float s) noexcept { return {a.x * s, a.y * s, a.z * s}; }
    constexpr Vec3 operator*(float s, Vec3 a) noexcept { return a * s; }
    constexpr Vec3 operator/(Vec3 a, float s) noexcept { return {a.x / s, a.y / s, a.z / s}; }
    constexpr Vec3& operator+=(Vec3& a, Vec3 b) noexcept { return a = a + b; }
    constexpr Vec3& operator-=(Vec3& a, Vec3 b) noexcept { return a = a - b; }
    constexpr Vec3& operator*=(Vec3& a, float s) noexcept { return a = a * s; }

    constexpr float dot(Vec3 a, Vec3 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z; }
    constexpr float length_squared(Vec3 a) noexcept { return dot(a, a); }

    // 右手系叉积：cross(X, Y) == Z
    constexpr Vec3 cross(const Vec3 a, const Vec3 b) noexcept
    {
        return {.x = a.y * b.z - a.z * b.y, .y = a.z * b.x - a.x * b.z, .z = a.x * b.y - a.y * b.x};
    }

    // Vec4 utils

    constexpr Vec4 operator+(Vec4 a, Vec4 b) noexcept { return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w}; }
    constexpr Vec4 operator-(Vec4 a, Vec4 b) noexcept { return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w}; }
    constexpr Vec4 operator*(Vec4 a, float s) noexcept { return {a.x * s, a.y * s, a.z * s, a.w * s}; }
    constexpr Vec4 operator*(float s, Vec4 a) noexcept { return a * s; }

    constexpr float dot(Vec4 a, Vec4 b) noexcept { return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w; }

    // 非 constexpr：std::sqrt 在 C++20 不是 constexpr

    inline float length(const Vec2 a) noexcept { return std::sqrt(length_squared(a)); }

    inline float length(const Vec3 a) noexcept { return std::sqrt(length_squared(a)); }

    /**
     * 2D向量的归一化：把任意非零向量缩放成长度为 1 的单位向量（方向不变、长度变为 1）
     *
     * 归一化只改变长度、不改变方向，用于点积求夹角、求方向、投影等要求单位向量的场合
     *
     * @param vec 待归一化的 2D 向量
     * @return 归一化后的单位向量；若输入为零向量，返回 Vec2{}
     * @note （1）零向量（|a| = 0）无法归一化，此时返回零向量 Vec2{} 以避免除零产生 NaN
     * @note （2）判断条件是 len > 0，仅防严格零，不防极小的非零向量，输入可能接近零，调用方需自行加 epsilon 保护
     */
    inline Vec2 normalized(const Vec2 vec) noexcept
    {
        const float len = length(vec);
        return len > 0.0f ? vec / len : Vec2{};
    }

    /**
     * 3D向量的归一化：把任意非零向量缩放成长度为 1 的单位向量（方向不变、长度变为 1）
     *
     * 归一化只改变长度、不改变方向，用于点积求夹角、求方向、叉积求旋转轴、物理方向计算等要求单位向量的场合
     *
     * @param vec 待归一化的 3D 向量
     * @return 归一化后的单位向量；若输入为零向量，返回 Vec3{}
     * @note （1）零向量（|a| = 0）无法归一化，此时返回零向量 Vec3{} 以避免除零产生 NaN
     * @note （2）判断条件是 len > 0，仅防严格零，不防极小的非零向量
     */
    inline Vec3 normalized(const Vec3 vec) noexcept
    {
        const float len = length(vec);
        return len > 0.0f ? vec / len : Vec3{};
    }

} // namespace taixu::core
