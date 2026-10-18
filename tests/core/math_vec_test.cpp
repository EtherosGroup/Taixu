#include <doctest/doctest.h>

#include "core/test_util.hpp"
#include "taixu/core/math/vec.hpp"

using namespace taixu::core;

TEST_CASE("叉积符合右手系")
{
    const Vec3 x{1.0f, 0.0f, 0.0f};
    const Vec3 y{0.0f, 1.0f, 0.0f};
    const Vec3 z{0.0f, 0.0f, 1.0f};

    CHECK(txu_test::approx(cross(x, y), z));
    CHECK(txu_test::approx(cross(y, z), x));
    CHECK(txu_test::approx(cross(z, x), y));

    // 反交换
    CHECK(txu_test::approx(cross(y, x), -z));
}

TEST_CASE("点积与长度")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{4.0f, -5.0f, 6.0f};

    CHECK(txu_test::approx(dot(a, b), 12.0f));
    CHECK(txu_test::approx(length(Vec3{3.0f, 4.0f, 0.0f}), 5.0f));
    CHECK(txu_test::approx(length_squared(a), 14.0f));
}

TEST_CASE("归一化")
{
    const Vec3 n = normalized(Vec3{0.0f, 0.0f, 7.0f});
    CHECK(txu_test::approx(n, Vec3{0.0f, 0.0f, 1.0f}));
    CHECK(txu_test::approx(length(n), 1.0f));

    // 零向量返回零向量，不做除零
    CHECK(txu_test::approx(normalized(Vec3{}), Vec3{}));
}

TEST_CASE("向量算术")
{
    const Vec3 a{1.0f, 2.0f, 3.0f};
    const Vec3 b{0.5f, 0.5f, 0.5f};

    CHECK(txu_test::approx(a + b, Vec3{1.5f, 2.5f, 3.5f}));
    CHECK(txu_test::approx(a - b, Vec3{0.5f, 1.5f, 2.5f}));
    CHECK(txu_test::approx(a * 2.0f, Vec3{2.0f, 4.0f, 6.0f}));
    CHECK(txu_test::approx(a / 2.0f, Vec3{0.5f, 1.0f, 1.5f}));
    CHECK(txu_test::approx(-a, Vec3{-1.0f, -2.0f, -3.0f}));
}
