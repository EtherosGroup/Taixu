#pragma once

/*
 * 3D变换（Transform）组件
 *
 * 变换：位置、旋转、缩放
 * 合成顺序为 T * R * S：先缩放，再旋转，最后平移
 * 变换按列向量右乘：v' = T * R * S * v
 *
 * Transform 用 位置 + 四元数旋转 + 缩放 描述物体姿态
 * to_mat4 按 T * R * S 合成出供顶点变换使用的 model 矩阵
 * normal_matrix 用 R * S^-T（旋转 × 缩放的逆转置）正确变换法线，避免非等比缩放下光照出错
 *
 * 这里是"数学基础函数"到"场景物体/渲染管线"之间的组装层
 *
 * 注意：该表示对乘法不封闭——父节点含非等比缩放且子节点含旋转时，两者的世界矩阵会引入剪切，不再可分解回 Transform
 *
 */

#include "taixu/core/math/mat.hpp"
#include "taixu/core/math/quat.hpp"
#include "taixu/core/math/vec.hpp"

namespace taixu::core
{
    /**
     * 3D变换结构体
     */
    struct Transform
    {
        /**
         * 位置（平移）
         */
        Vec3 position{};
        /**
         * 旋转（四元数，默认不转）
         */
        Quat rotation = Quat::identity();
        /**
         * 缩放，默认 1
         */
        Vec3 scale{.x = 1.0f, .y = 1.0f, .z = 1.0f};
    };

    /**
     * 把 Transform 合成一个 4×4 矩阵
     * @note 用途：顶点/物体变换：顶点着色器的 model 矩阵、父子变换链、碰撞包围盒变换
     * @param t 3D转换组件
     * @return 4x4矩阵
     */
    inline Mat4 to_mat4(const Transform& t) noexcept
    {
        return Mat4::translation(t.position) * to_mat4(t.rotation) * Mat4::scaling(t.scale);
    }

    /*
     * 这是光照正确性的关键
     * 法线不能像普通顶点那样直接用 T * R * S 变换，原因：
     * - 顶点是位置，按 M 变换（w=1）
     * - 法线是方向，按 (M^-1)^T 变换（w=0）
     *
     * 为什么？
     * 若表面在某点切线为 t，法线 n 满足 ⟨n, t⟩ = 0，缩放后切线变了，法线必须相应调整才能保持垂直
     * 非等比缩放（比如压扁）会让直接用原矩阵变换的法线歪掉，光照出现明暗错误
     * 并且 这里只用了rotation * scale，没有平移：因为法线是方向，平移不影响方向
     *
     * 所以最终得到 R * S^-1：旋转正常，缩放取逆转置（等比缩放下退化为 1/s，方向不变；非等比下校正各轴）
     * 一般公式是逆转置 (R * S)^-T = R * S^-T
     * 因为 R 正交（R^-T = R）、S 对角（S^-T = S^-1），所以化简成 R * S^-1
     * 换句话说：旋转照原样，缩放取倒数
     * 这正是"逆转置"的来历
     *
     * 用途：法线变换
     * 使用场景：光照计算（法线贴图、Phong/PBR）、表面朝向
     *
     */
    /**
     * 法线变换矩阵
     * @note （1）用途：法线变换：光照计算（法线贴图、Phong/PBR）、表面朝向
     * @note （2）变换后的法线长度不再是 1，使用前必须重新归一
     * @note （3）用法：transform_direction(normal_matrix(t), n)，再 normalized()化
     * @param t 3D变换组件
     * @return 4x4 法线变换矩阵
     */
    inline Mat4 normal_matrix(const Transform& t) noexcept
    {
        return transposed(inverse(to_mat4(t.rotation) * Mat4::scaling(t.scale)));
    }

} // namespace taixu::core
