#include <doctest/doctest.h>

#include "core/test_util.hpp"
#include "taixu/core/math/aabb.hpp"

using namespace taixu::core;

namespace
{
    constexpr Aabb kUnitBox{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
} // namespace

TEST_CASE("射线命中包围盒")
{
    const Ray ray{{0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, -1.0f}};
    float t_min = 0.0f;
    float t_max = 0.0f;

    REQUIRE(intersect(ray, kUnitBox, t_min, t_max));
    CHECK(txu_test::approx(t_min, 4.0f));
    CHECK(txu_test::approx(t_max, 6.0f));
}

TEST_CASE("射线错过包围盒")
{
    float t_min = 0.0f;
    float t_max = 0.0f;

    // 侧面擦过
    CHECK_FALSE(intersect(Ray{.origin = {2.0f, 0.0f, 5.0f}, .direction = {.x = 0.0f, .y = 0.0f, .z = -1.0f}}, kUnitBox, t_min, t_max));
    // 平行且落在 slab 外
    CHECK_FALSE(intersect(Ray{.origin = {0.0f, 5.0f, 0.0f}, .direction = {.x = 1.0f, .y = 0.0f, .z = 0.0f}}, kUnitBox, t_min, t_max));
    // 盒在射线起点之后
    CHECK_FALSE(intersect(Ray{.origin = {5.0f, 0.0f, 0.0f}, .direction = {.x = 1.0f, .y = 0.0f, .z = 0.0f}}, kUnitBox, t_min, t_max));
}

TEST_CASE("射线起点在盒内")
{
    const Ray ray{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
    float t_min = 0.0f;
    float t_max = 0.0f;

    REQUIRE(intersect(ray, kUnitBox, t_min, t_max));
    CHECK(t_min < 0.0f);
    CHECK(txu_test::approx(t_max, 1.0f));
}

TEST_CASE("平行射线落在 slab 内")
{
    const Ray ray{{-5.0f, 0.5f, 0.25f}, {1.0f, 0.0f, 0.0f}};
    float t_min = 0.0f;
    float t_max = 0.0f;

    REQUIRE(intersect(ray, kUnitBox, t_min, t_max));
    CHECK(txu_test::approx(t_min, 4.0f));
    CHECK(txu_test::approx(t_max, 6.0f));
}

TEST_CASE("包围盒的合并与扩展")
{
    Aabb b = Aabb::empty(); // 反向无穷，第一次扩展后成为唯一点
    b = expanded(b, {1.0f, 2.0f, 3.0f});
    CHECK(txu_test::approx(b.min, Vec3{1.0f, 2.0f, 3.0f}));
    CHECK(txu_test::approx(b.max, Vec3{1.0f, 2.0f, 3.0f}));

    b = expanded(b, {-1.0f, 5.0f, 0.0f});
    CHECK(txu_test::approx(b.min, Vec3{-1.0f, 2.0f, 0.0f}));
    CHECK(txu_test::approx(b.max, Vec3{1.0f, 5.0f, 3.0f}));

    CHECK(txu_test::approx(center(kUnitBox), Vec3{}));
    CHECK(txu_test::approx(extent(kUnitBox), Vec3{2.0f, 2.0f, 2.0f}));
    CHECK(contains(kUnitBox, Vec3{0.5f, -0.5f, 0.0f}));
    CHECK_FALSE(contains(kUnitBox, Vec3{1.5f, 0.0f, 0.0f}));

    const Aabb other{{2.0f, 2.0f, 2.0f}, {3.0f, 3.0f, 3.0f}};
    const Aabb m = merged(kUnitBox, other);
    CHECK(txu_test::approx(m.min, Vec3{-1.0f, -1.0f, -1.0f}));
    CHECK(txu_test::approx(m.max, Vec3{3.0f, 3.0f, 3.0f}));
}
