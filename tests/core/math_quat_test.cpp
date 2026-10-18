#include <doctest/doctest.h>

#include "core/test_util.hpp"
#include "taixu/core/math/quat.hpp"

using namespace taixu::core;

TEST_CASE("绕 Y 轴 90 度把 +X 转到 -Z")
{
    const Quat q = from_axis_angle({0.0f, 1.0f, 0.0f}, kHalfPi);

    CHECK(txu_test::approx(rotate(q, Vec3{1.0f, 0.0f, 0.0f}), Vec3{0.0f, 0.0f, -1.0f}));
    CHECK(txu_test::approx(rotate(q, Vec3{0.0f, 1.0f, 0.0f}), Vec3{0.0f, 1.0f, 0.0f}));
}

TEST_CASE("四元数与矩阵表示同一个旋转")
{
    const Quat q = from_axis_angle(normalized(Vec3{1.0f, 2.0f, -0.5f}), 0.9f);
    const Mat4 m = to_mat4(q);

    CHECK(txu_test::approx(to_mat4(from_axis_angle({0.0f, 1.0f, 0.0f}, kHalfPi)), Mat4::rotation_y(kHalfPi)));

    const Vec3 samples[] = {Vec3{1.0f, 0.0f, 0.0f}, Vec3{0.0f, 1.0f, 0.0f}, Vec3{2.0f, -1.0f, 3.0f}};
    for (const Vec3 v : samples)
    {
        CHECK(txu_test::approx(rotate(q, v), transform_direction(m, v)));
    }
}

TEST_CASE("四元数乘法等于旋转的复合")
{
    const Quat a = from_axis_angle({0.0f, 1.0f, 0.0f}, kHalfPi);
    const Quat b = from_axis_angle({1.0f, 0.0f, 0.0f}, kHalfPi);
    const Vec3 v{1.0f, 0.0f, 0.0f};

    // q2 * q1 表示先施加 q1，再施加 q2
    CHECK(txu_test::approx(rotate(b * a, v), rotate(b, rotate(a, v))));
}

TEST_CASE("共轭即逆")
{
    const Quat q = from_axis_angle({1.0f, 0.0f, 0.0f}, 1.2f);

    CHECK(txu_test::approx(q * conjugate(q), Quat::identity()));
    CHECK(txu_test::approx(rotate(conjugate(q), rotate(q, Vec3{1.0f, 2.0f, 3.0f})), Vec3{1.0f, 2.0f, 3.0f}));
}

TEST_CASE("归一化")
{
    const Quat q = normalized(Quat{2.0f, 0.0f, 0.0f, 0.0f});
    CHECK(txu_test::approx(length(q), 1.0f));
    CHECK(txu_test::approx(q, Quat{1.0f, 0.0f, 0.0f, 0.0f}));

    // 零四元数退化为单位旋转
    CHECK(txu_test::approx(normalized(Quat{0.0f, 0.0f, 0.0f, 0.0f}), Quat::identity()));
}

TEST_CASE("球面插值走短弧")
{
    const Quat a = Quat::identity();
    const Quat b = from_axis_angle({0.0f, 1.0f, 0.0f}, kHalfPi);

    // 中点是绕 Y 轴 45 度
    const Quat mid = slerp(a, b, 0.5f);
    const Vec3 rotated = rotate(mid, Vec3{1.0f, 0.0f, 0.0f});
    CHECK(txu_test::approx(rotated, Vec3{0.70710678f, 0.0f, -0.70710678f}));
    CHECK(txu_test::approx(length(mid), 1.0f));

    // 端点与参数 0 / 1
    CHECK(txu_test::approx(rotate(slerp(a, b, 0.0f), Vec3{1.0f, 0.0f, 0.0f}), Vec3{1.0f, 0.0f, 0.0f}));
    CHECK(txu_test::approx(rotate(slerp(a, b, 1.0f), Vec3{1.0f, 0.0f, 0.0f}), Vec3{0.0f, 0.0f, -1.0f}));

    // b 的等价表示取反后，结果不变
    const Quat b_neg{-b.x, -b.y, -b.z, -b.w};
    CHECK(txu_test::approx(slerp(a, b, 0.5f), slerp(a, b_neg, 0.5f)));
}
