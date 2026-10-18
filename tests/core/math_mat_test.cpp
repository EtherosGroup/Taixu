#include <doctest/doctest.h>

#include "core/test_util.hpp"
#include "taixu/core/math/mat.hpp"

using namespace taixu::core;

TEST_CASE("单位矩阵是乘法的幺元")
{
    const Mat4 m = Mat4::translation({1.0f, 2.0f, 3.0f}) * Mat4::scaling({2.0f, 2.0f, 2.0f});
    CHECK(txu_test::approx(Mat4::identity() * m, m));
    CHECK(txu_test::approx(m * Mat4::identity(), m));
}

TEST_CASE("存储是行主序，平移在第四列")
{
    const Mat4 t = Mat4::translation({7.0f, 8.0f, 9.0f});

    // m[row][col] 的内存偏移为 row * 4 + col
    CHECK(txu_test::approx(t[0][3], 7.0f));
    CHECK(txu_test::approx(t[1][3], 8.0f));
    CHECK(txu_test::approx(t[2][3], 9.0f));

    const float* flat = &t.mat[0][0];
    CHECK(txu_test::approx(flat[3], 7.0f));
    CHECK(txu_test::approx(flat[7], 8.0f));
    CHECK(txu_test::approx(flat[11], 9.0f));
}

TEST_CASE("点变换含平移，方向变换不含平移")
{
    const Mat4 t = Mat4::translation({1.0f, 2.0f, 3.0f});

    CHECK(txu_test::approx(transform_point(t, Vec3{}), Vec3{1.0f, 2.0f, 3.0f}));
    CHECK(txu_test::approx(transform_direction(t, Vec3{1.0f, 1.0f, 1.0f}), Vec3{1.0f, 1.0f, 1.0f}));
}

TEST_CASE("绕 Y 轴 +90 度把 +X 转到 -Z，符合右手系")
{
    const Mat4 ry = Mat4::rotation_y(kHalfPi);

    CHECK(txu_test::approx(transform_direction(ry, Vec3{1.0f, 0.0f, 0.0f}), Vec3{0.0f, 0.0f, -1.0f}));
    CHECK(txu_test::approx(transform_direction(ry, Vec3{0.0f, 0.0f, -1.0f}), Vec3{-1.0f, 0.0f, 0.0f}));
    CHECK(txu_test::approx(transform_direction(ry, Vec3{0.0f, 1.0f, 0.0f}), Vec3{0.0f, 1.0f, 0.0f}));

    const Mat4 ry_neg = Mat4::rotation_y(-kHalfPi);
    CHECK(txu_test::approx(transform_direction(ry_neg, Vec3{1.0f, 0.0f, 0.0f}), Vec3{0.0f, 0.0f, 1.0f}));
}

TEST_CASE("绕 X 与 Z 轴的旋转方向")
{
    CHECK(txu_test::approx(transform_direction(Mat4::rotation_x(kHalfPi), Vec3{0.0f, 1.0f, 0.0f}),
                           Vec3{0.0f, 0.0f, 1.0f}));
    CHECK(txu_test::approx(transform_direction(Mat4::rotation_z(kHalfPi), Vec3{1.0f, 0.0f, 0.0f}),
                           Vec3{0.0f, 1.0f, 0.0f}));
}

TEST_CASE("复合顺序为先右后左")
{
    const Mat4 t = Mat4::translation({10.0f, 0.0f, 0.0f});
    const Mat4 r = Mat4::rotation_y(kHalfPi);

    // T * R：先旋转，再平移
    CHECK(txu_test::approx(transform_point(t * r, Vec3{1.0f, 0.0f, 0.0f}), Vec3{10.0f, 0.0f, -1.0f}));
    // R * T：先平移，再旋转
    CHECK(txu_test::approx(transform_point(r * t, Vec3{1.0f, 0.0f, 0.0f}), Vec3{0.0f, 0.0f, -11.0f}));
}

TEST_CASE("转置")
{
    Mat4 m = Mat4::identity();
    m[0][1] = 2.0f;
    m[0][2] = 3.0f;
    m[1][3] = 4.0f;

    const Mat4 t = transposed(m);
    CHECK(txu_test::approx(t[1][0], 2.0f));
    CHECK(txu_test::approx(t[2][0], 3.0f));
    CHECK(txu_test::approx(t[3][1], 4.0f));
    CHECK(txu_test::approx(transposed(t), m));
}

TEST_CASE("求逆")
{
    const Mat4 m = Mat4::translation({1.0f, 2.0f, 3.0f}) * Mat4::rotation_y(0.7f) * Mat4::scaling({2.0f, 4.0f, 8.0f});

    CHECK(txu_test::approx(m * inverse(m), Mat4::identity(), 1.0e-4f));
    CHECK(txu_test::approx(inverse(m) * m, Mat4::identity(), 1.0e-4f));

    // 逆变换把点送回原处
    const Vec3 p{5.0f, -3.0f, 2.0f};
    CHECK(txu_test::approx(transform_point(inverse(m), transform_point(m, p)), p));
}

TEST_CASE("奇异矩阵求逆返回全零")
{
    Mat4 singular{}; // 全零
    const Mat4 inv = inverse(singular);
    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            CHECK(inv[i][j] == 0.0f);
        }
    }
}
