#pragma once

// 测试用的近似比较

#include <doctest/doctest.h>

#include <cmath>

#include "taixu/core/math.hpp"

namespace txu_test
{

    inline bool approx(float a, float b, float eps = 1.0e-4f) noexcept { return std::fabs(a - b) <= eps; }

    inline bool approx(taixu::core::Vec2 a, taixu::core::Vec2 b, float eps = 1.0e-4f) noexcept
    {
        return approx(a.x, b.x, eps) && approx(a.y, b.y, eps);
    }

    inline bool approx(taixu::core::Vec3 a, taixu::core::Vec3 b, float eps = 1.0e-4f) noexcept
    {
        return approx(a.x, b.x, eps) && approx(a.y, b.y, eps) && approx(a.z, b.z, eps);
    }

    inline bool approx(taixu::core::Vec4 a, taixu::core::Vec4 b, float eps = 1.0e-4f) noexcept
    {
        return approx(a.x, b.x, eps) && approx(a.y, b.y, eps) && approx(a.z, b.z, eps) && approx(a.w, b.w, eps);
    }

    inline bool approx(const taixu::core::Mat4& a, const taixu::core::Mat4& b, float eps = 1.0e-4f) noexcept
    {
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                if (!approx(a.mat[i][j], b.mat[i][j], eps))
                {
                    return false;
                }
            }
        }
        return true;
    }

    inline bool approx(taixu::core::Quat a, taixu::core::Quat b, float eps = 1.0e-4f) noexcept
    {
        return approx(a.x, b.x, eps) && approx(a.y, b.y, eps) && approx(a.z, b.z, eps) && approx(a.w, b.w, eps);
    }

    // 裁剪空间坐标的透视除法，得到 NDC
    inline taixu::core::Vec3 to_ndc(const taixu::core::Mat4& m, taixu::core::Vec3 p) noexcept
    {
        const taixu::core::Vec4 c = m * taixu::core::Vec4{p.x, p.y, p.z, 1.0f};
        return {c.x / c.w, c.y / c.w, c.z / c.w};
    }

} // namespace txu_test
