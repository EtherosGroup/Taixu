#include <doctest/doctest.h>

#include "core/test_util.hpp"
#include "taixu/core/math/projection.hpp"

using namespace taixu::core;

namespace
{
    constexpr float kNear = 0.1f;
    constexpr float kFar = 100.0f;
} // namespace

TEST_CASE("透视投影的深度范围是 0 到 1")
{
    const Mat4 p = perspective(kHalfPi, 1.0f, kNear, kFar);

    const Vec4 near_clip = p * Vec4{0.0f, 0.0f, -kNear, 1.0f};
    CHECK(txu_test::approx(near_clip.z / near_clip.w, 0.0f));

    const Vec4 far_clip = p * Vec4{0.0f, 0.0f, -kFar, 1.0f};
    CHECK(txu_test::approx(far_clip.z / far_clip.w, 1.0f));

    // 深度单调递增
    const float z20 = txu_test::to_ndc(p, Vec3{0.0f, 0.0f, -20.0f}).z;
    const float z5 = txu_test::to_ndc(p, Vec3{0.0f, 0.0f, -5.0f}).z;
    CHECK(z5 > 0.0f);
    CHECK(z20 > z5);
    CHECK(z20 < 1.0f);
}

TEST_CASE("裁剪空间的 w 是视空间深度")
{
    const Mat4 p = perspective(kHalfPi, 1.0f, kNear, kFar);

    const Vec4 c = p * Vec4{3.0f, 4.0f, -7.0f, 1.0f};
    CHECK(txu_test::approx(c.w, 7.0f));
}

TEST_CASE("透视投影翻转 Y，使世界的上方对应屏幕上方")
{
    const Mat4 p = perspective(kHalfPi, 1.0f, kNear, kFar);

    // Vulkan 的 NDC 中 Y 向下，世界坐标 +Y 应映射到 NDC 的负 Y
    CHECK(txu_test::to_ndc(p, Vec3{0.0f, 1.0f, -5.0f}).y < 0.0f);
    CHECK(txu_test::to_ndc(p, Vec3{0.0f, -1.0f, -5.0f}).y > 0.0f);

    // X 不翻转
    CHECK(txu_test::to_ndc(p, Vec3{1.0f, 0.0f, -5.0f}).x > 0.0f);
    CHECK(txu_test::to_ndc(p, Vec3{-1.0f, 0.0f, -5.0f}).x < 0.0f);
}

TEST_CASE("宽高比只压缩 X")
{
    const Mat4 p1 = perspective(kHalfPi, 1.0f, kNear, kFar);
    const Mat4 p2 = perspective(kHalfPi, 2.0f, kNear, kFar);

    const Vec3 a = txu_test::to_ndc(p1, Vec3{1.0f, 0.0f, -5.0f});
    const Vec3 b = txu_test::to_ndc(p2, Vec3{1.0f, 0.0f, -5.0f});
    CHECK(txu_test::approx(b.x, a.x * 0.5f));
    CHECK(txu_test::approx(b.y, a.y));
}

TEST_CASE("正交投影的深度范围与 Y 翻转")
{
    const Mat4 o = orthographic(-1.0f, 1.0f, -1.0f, 1.0f, 1.0f, 11.0f);

    CHECK(txu_test::approx(txu_test::to_ndc(o, Vec3{0.0f, 0.0f, -1.0f}).z, 0.0f));
    CHECK(txu_test::approx(txu_test::to_ndc(o, Vec3{0.0f, 0.0f, -11.0f}).z, 1.0f));

    // 视景体上边界映射到 NDC 的 -1
    CHECK(txu_test::approx(txu_test::to_ndc(o, Vec3{0.0f, 1.0f, -1.0f}).y, -1.0f));
    CHECK(txu_test::approx(txu_test::to_ndc(o, Vec3{0.0f, -1.0f, -1.0f}).y, 1.0f));

    // 视景体左右边界
    CHECK(txu_test::approx(txu_test::to_ndc(o, Vec3{1.0f, 0.0f, -1.0f}).x, 1.0f));
    CHECK(txu_test::approx(txu_test::to_ndc(o, Vec3{-1.0f, 0.0f, -1.0f}).x, -1.0f));
}

TEST_CASE("视图矩阵把相机前方的点放到 -Z")
{
    const Mat4 v = look_at({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    // 世界原点在相机前方 5 个单位
    CHECK(txu_test::approx(transform_point(v, Vec3{}), Vec3{0.0f, 0.0f, -5.0f}));
    CHECK(txu_test::approx(transform_point(v, Vec3{0.0f, 0.0f, 5.0f}), Vec3{}));

    // 世界坐标轴与相机坐标轴重合
    CHECK(txu_test::approx(transform_direction(v, Vec3{1.0f, 0.0f, 0.0f}), Vec3{1.0f, 0.0f, 0.0f}));
    CHECK(txu_test::approx(transform_direction(v, Vec3{0.0f, 1.0f, 0.0f}), Vec3{0.0f, 1.0f, 0.0f}));
    CHECK(txu_test::approx(transform_direction(v, Vec3{0.0f, 0.0f, -1.0f}), Vec3{0.0f, 0.0f, -1.0f}));
}

TEST_CASE("视图矩阵保持右手系")
{
    const Mat4 v = look_at({3.0f, 4.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});

    // 旋转部分是正交的，长度不变
    const Vec3 d = normalized(Vec3{1.0f, -2.0f, 0.5f});
    CHECK(txu_test::approx(length(transform_direction(v, d)), 1.0f));
}
