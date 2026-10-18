#pragma once

/*
 * AABB，即 轴对齐包围盒（Axis-Aligned Bounding Box）
 */

#include <cmath>
#include <limits>

#include "taixu/core/math/constants.hpp"
#include "taixu/core/math/vec.hpp"

namespace taixu::core
{
    /**
     * AABB结构体
     */
    struct Aabb
    {
        /**
         * 盒子的最小点
         */
        Vec3 min{};

        /**
         * 盒子的最大点
         */
        Vec3 max{};

        /**
         * 创建一个空的 AABB 结构体
         *
         * 作为合并包围盒的单位元：从 empty() 开始，不断用逐分量 min/max 把新盒子并进来，+∞ 会被任何有限值取代、−∞ 同理
         * @note （1）min 取正无穷，max 取负无穷
         * @note （2）Aabb{} 与 empty() 语义不同：默认构造得到 min = max = (0, 0, 0)，是一个位于原点的退化盒，contains(Aabb{}, 原点) 为 true。需要“空盒子”当起点时必须显式写 Aabb::empty()
         * @note （3）empty() 只可作为 expanded() / merged() 的起始种子：center(empty()) 三个分量都是 NaN（(inf + −inf) * 0.5），extent(empty()) 三个分量都是 −inf
         * @return 空的包围盒，min 为正无穷、max 为负无穷
         */
        static constexpr Aabb empty() noexcept
        {
            constexpr float inf = std::numeric_limits<float>::infinity();
            return {.min = {.x = inf, .y = inf, .z = inf}, .max = {.x = -inf, .y = -inf, .z = -inf}};
        }
    };

    /**
     * 射线
     */
    struct Ray
    {
        /**
         * 起点（射线的出发点）
         */
        Vec3 origin{};
        /**
         * 方向（射线的朝向）
         */
        Vec3 direction{};
    };

    /**
     * 计算包围盒中心点
     * @param box AABB 结构体
     * @return 包围盒的几何中心（min 与 max 的中点）
     */
    constexpr Vec3 center(const Aabb& box) noexcept { return (box.min + box.max) * 0.5f; }

    /**
     * 计算包围盒尺寸（extent）
     * @param box AABB 结构体
     * @return 包围盒在 x、y、z 三个轴上的跨度，即 max - min 的逐分量结果
     */
    constexpr Vec3 extent(const Aabb& box) noexcept { return box.max - box.min; }

    /**
     * 判断点是否在包围盒内部（含边界）
     * @param box AABB 结构体
     * @param point Vec3 点
     * @note 含边界
     * @return 点在盒内（含边界）返回 true，否则返回 false
     */
    constexpr bool contains(const Aabb& box, const Vec3 point) noexcept
    {
        return point.x >= box.min.x && point.x <= box.max.x && point.y >= box.min.y && point.y <= box.max.y && point.z >= box.min.z && point.z <= box.max.z;
    }

    /**
     * 把点并入包围盒
     * @param box AABB 结构体
     * @param point 要并入的 Vec3 点
     * @note 可以用 Aabb::empty() 作为起点逐个并入点，得到一组点的包围盒
     * @return 能同时容纳原盒与点的最小包围盒
     */
    constexpr Aabb expanded(const Aabb& box, Vec3 point) noexcept
    {
        Aabb r = box;
        if (point.x < r.min.x) r.min.x = point.x;
        if (point.y < r.min.y) r.min.y = point.y;
        if (point.z < r.min.z) r.min.z = point.z;
        if (point.x > r.max.x) r.max.x = point.x;
        if (point.y > r.max.y) r.max.y = point.y;
        if (point.z > r.max.z) r.max.z = point.z;
        return r;
    }
    
    /**
     * 合并两个包围盒
     * @param box_a 第一个 AABB 结构体
     * @param box_b 第二个 AABB 结构体
     * @note 与 expanded() 一样，Aabb::empty() 是这一步的单位元
     * @return 能同时容纳两个盒的最小包围盒
     */
    constexpr Aabb merged(const Aabb& box_a, const Aabb& box_b) noexcept
    {
        return {
            .min = {.x = box_a.min.x < box_b.min.x ? box_a.min.x : box_b.min.x, .y = box_a.min.y < box_b.min.y ? box_a.min.y : box_b.min.y,
                .z = box_a.min.z < box_b.min.z ? box_a.min.z : box_b.min.z},
            .max = {.x = box_a.max.x > box_b.max.x ? box_a.max.x : box_b.max.x, .y = box_a.max.y > box_b.max.y ? box_a.max.y : box_b.max.y,
                .z = box_a.max.z > box_b.max.z ? box_a.max.z : box_b.max.z}
        };
    }

    /*
     * 【背景】
     * 什么是 slab，什么是 slab 法
     * slab 的中文直译是 “板、厚片”。在射线求交里，一个 slab 指的是由一对平行平面夹出来的无限厚板。对 AABB 来说，盒子在 x、y、z 三个轴上各有一对边界平面。比如 x 轴：
     * - 左平面 x = box.min.x
     * - 右平面 x = box.max.x
     * 这两个平面之间的无限空间，就是 x 轴方向的一个 slab，同理有 y-slab、z-slab
     *
     * 那么这个板跟射线求交有什么关系呢？观察一下我们的包围盒（AABB），可以联想到：包围盒 正好是三个 slab 的交集！
     * 因此，要判断一个射线是否穿过了某个包围盒，就只需要判断该射线是否同时穿过了这个包围盒的三个 slab，对吧，这就是 slab 法求交
     *
     * 但是需要注意的是，平行轴要单独处理：
     * 几何上，这时射线要么永远在这个 slab 内（起点坐标落在 [min, max] 之间），要么永远在 slab 外（直接不相交）
     * 所以代码单独判断 origin[axis] 是否在区间内，在就跳过该轴，不在就返回 false
     *
     * 为什么不能直接做除法：IEEE 浮点下 1.0f / direction[axis] 不报错，得到的是 ±inf；
     * 真正的问题在紧接着的 (lo - origin) * inv_d —— 起点恰好落在边界平面上时是 0 * inf，结果是 NaN，
     * 而 NaN 参与任何比较都为 false，区间会被静默算错，不崩溃、不报错
     * 代码用的是 |direction[axis]| < kEpsilon（1e-5）而不是 == 0，所以拦下的不只是精确的零，
     * 还有接近平行的射线：那种情况下 1/d 会大到 1e5 以上，slab 边界的计算已经不可信
     * 严格说，|d| < kEpsilon 但非零时射线并不是"永远"待在 slab 里，只是漂移极慢，
     * 代码把它当平行处理——这就是上面说的保守取舍：宁可漏判，不误判
     */
    /**
     * 判断射线是否与包围盒相交
     * 射线与轴对齐包围盒（AABB）求交，使用 slab 法
     *
     * @param ray 射线（origin + t * direction，t ≥ 0）
     * @param box 轴对齐包围盒
     * @param t_min 输出：进入盒子的参数 t；若射线起点在盒内，该值可能为负
     * @param t_max 输出：离开盒子的参数 t
     *
     * @note （1）允许 direction 的某个分量为 0（射线平行于某轴），此时该轴单独判断 origin 是否落在 [min, max] 区间内，不在则直接判定不相交
     * @note （2）t_min / t_max 仅在函数返回 true 时有意义；返回 false 时其值无定义
     * @note （3）射线单向（t ≥ 0），盒子整体位于起点之后（t_max < 0）时不算命中
     * @note （4）擦过边界（恰好切到面、边或角）算作命中
     * @note （5）direction 不可为零向量。传入零向量时三个轴都走平行分支，t_min / t_max 保持初值 ±inf，函数返回 true，得到一个 (−inf, +inf) 的无效"命中"
     *
     * @return 射线与盒子相交返回 true，此时 [t_min, t_max] 为射线落在盒内的参数区间；否则返回 false
     */
    inline bool intersect(const Ray& ray, const Aabb& box, float& t_min, float& t_max) noexcept
    {
        t_min = -std::numeric_limits<float>::infinity();
        t_max = std::numeric_limits<float>::infinity();

        const float origin[3] = {ray.origin.x, ray.origin.y, ray.origin.z};
        const float direction[3] = {ray.direction.x, ray.direction.y, ray.direction.z};
        const float lo[3] = {box.min.x, box.min.y, box.min.z};
        const float hi[3] = {box.max.x, box.max.y, box.max.z};

        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(direction[axis]) < kEpsilon)
            {
                // 射线与该轴平行，起点必须在 slab 内
                if (origin[axis] < lo[axis] || origin[axis] > hi[axis])
                {
                    return false;
                }
                continue;
            }

            const float inv_d = 1.0f / direction[axis];
            float t0 = (lo[axis] - origin[axis]) * inv_d;
            float t1 = (hi[axis] - origin[axis]) * inv_d;
            if (t0 > t1)
            {
                const float tmp = t0;
                t0 = t1;
                t1 = tmp;
            }
            if (t0 > t_min) t_min = t0;
            if (t1 < t_max) t_max = t1;
            if (t_min > t_max)
            {
                return false;
            }
        }

        // 盒整体位于射线起点之后时不算命中
        return t_max >= 0.0f;
    }
} // namespace taixu::core
