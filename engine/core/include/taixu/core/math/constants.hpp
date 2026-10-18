#pragma once

/*
 * 数学常量与角度换算
 *
 * 全部使用弧度。角度换算只出现在边界（编辑器输入、资源文件）
 */

namespace taixu::core
{
    /**
     * π的近似值常量定义
     */
    inline constexpr float kPi = 3.14159265358979323846f;

    /**
     * 2π
     */
    inline constexpr float kTwoPi = 2.0f * kPi;

    /**
     * π/2
     */
    inline constexpr float kHalfPi = 0.5f * kPi;

    /**
     * 浮点比较与几何判定的默认容差
     */
    inline constexpr float kEpsilon = 1.0e-5f;

    /**
     * 角度转弧度
     * @param deg 角度制
     * @return 弧度制
     */
    constexpr float radians(const float deg) noexcept { return deg * (kPi / 180.0f); }

    /**
     * 弧度转角度
     * @param rad 弧度制
     * @return 角度制
     */
    constexpr float degrees(const float rad) noexcept { return rad * (180.0f / kPi); }

} // namespace taixu::core
