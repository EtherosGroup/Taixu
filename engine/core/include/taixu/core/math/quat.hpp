#pragma once

/*
 * 四元数，表示旋转
 *
 * 点 p = (x, y, z) 补成纯虚四元数 P = (x, y, z, 0)
 * Hamilton 乘法，最右边先作用：q 作用于向量等价于 R(q) * v
 * 单位四元数表示旋转，非单位四元数无意义，构造后自行归一化
 */

#include <cmath>

#include "taixu/core/math/mat.hpp"
#include "taixu/core/math/vec.hpp"

namespace taixu::core
{
    /*
     * 【背景】什么是四元数？干什么？怎么用？
     * 所谓四元数，直观上就是四个数元素的东西，四元数就是用四个数表示 3D 旋转的数学工具
     * 主要作用是与 4x4矩阵搭配使用：
     * - 四元数 = 一张“旋转指令卡”，写着“绕某轴转多少度”
     * - 矩阵 = 一台“旋转机器”，喂点进去就转
     * 形式为 q = w + xi + yj + zk
     * 其中 w 为实部，(x, y, z) 是虚部，i, j, k 是三个虚数单位，
     * 满足 i^2 = j^2 = k^2 = -1
     *
     * 四元数能做什么？
     * - 存储旋转
     * - 组合旋转
     * - 平滑插值
     * - 避免万向锁
     * - 保持正交性
     *
     * 原理解释：
     * 【1】普通复数 a + bi 有两个分量，乘法满足 i^2 = -1。四元数把它扩展到三个虚数单位：
     * q = w + xi + yj + zk
     * 其中 i^2 = j^2 = k^2 = ijk = -1
     * 注意 ijk = -1 这条规则——它决定了 i, j, k 之间怎么相乘，由此可以推出：
     * ij = k, ji = −k, jk = i, kj = −i, ki = j , ik = −j
     * 关键点：四元数乘法不交换（ij != ji）。这个“不交换”正是它能表示旋转的根本原因——因为 3D 旋转本身也不交换（先绕 X 转 90° 再绕 Y 转 90°，和反过来做，结果不同）
     *
     * 【2】而至于它为什么能表示旋转：
     * 把四元数写成“标量 + 向量”的形式
     * q = (w, v)   v = (x, y, z)
     * 对于单位四元数（w^2 + |v|^2 = 1），存在一个角度 θ 和单位轴 a，使得：
     * w = cos(θ/2)   v = a * sin(θ/2)
     * 也就是说，每个单位四元数都对应"绕轴 a 旋转 θ 角"；
     * 反过来一个旋转对应两个单位四元数 q 与 -q（二对一），这正是 slerp 要先判 dot 正负的原因
     * 为什么是 θ/2 而不是 θ？因为四元数旋转一个点要用两次乘法（下面会讲），半角正好在两次乘法后凑成完整角度
     *
     * 【3】四元数怎么旋转一个点
     * 点 p = (x, y, z) 补成纯虚四元数 P = (x, y, z, 0)，然后用单位四元数 q 做
     * P' = q * P * q^-1
     * 对单位四元数，q^-1 = q^*（共轭，即 (w, -v)），所以是 q * P * q*
     * （1）左乘 q 是在四元数空间里的旋转
     * （2）右乘 q* 保证结果仍然落在纯虚子空间（也就是仍然对应一个 3D 向量）
     * 两者合起来（共轭 q * P * q*）在纯虚子空间上的作用，恰好是绕 a 转 θ
     * 半角的原因：映射 q |-> (P |-> q * P * q*) 是 S^3 -> SO(3) 的二对一同态
     *
     * 严格证明需要展开四元数乘法，会得到一个和罗德里格斯旋转公式完全一致的 3x3 矩阵——也就是说，
     * q * P * q* 的虚部，恰好等于“用旋转矩阵乘 p”的结果。四元数旋转和矩阵旋转是同一件事的两种写法
     *
     * 默认值 (0, 0, 0, 1) 正好是单位四元数，表示“不旋转”，相当于矩阵里的单位矩阵，是旋转的初始状态。
     *
     */
    /**
     * 四元数结构体
     */
    struct Quat
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;

        /**
         * 默认构造的 Quat，即 (0, 0, 0, 1)，表示恒等旋转（不旋转）
         * @return 默认四元数
         */
        static constexpr Quat identity() noexcept { return {}; }
    };

    /**
     * 组合两个旋转
     * @param quat_a 四元数
     * @param quat_b 四元数
     * @note quat_a * quat_b 表示先施加 quat_b、再施加 quat_a（与矩阵乘法一致，最右边先作用）
     * @return 组合后的 四元数
     */
    constexpr Quat operator*(const Quat quat_a, const Quat quat_b) noexcept
    {
        return {.x = quat_a.w * quat_b.x + quat_a.x * quat_b.w + quat_a.y * quat_b.z - quat_a.z * quat_b.y, .y = quat_a.w * quat_b.y - quat_a.x * quat_b.z + quat_a.y * quat_b.w + quat_a.z * quat_b.x,
                .z = quat_a.w * quat_b.z + quat_a.x * quat_b.y - quat_a.y * quat_b.x + quat_a.z * quat_b.w, .w = quat_a.w * quat_b.w - quat_a.x * quat_b.x - quat_a.y * quat_b.y - quat_a.z * quat_b.z};
    }

    /**
     * 计算四元数模长（length / norm）
     * @note ||q|| = (x^2 + y^2 + z^2 + w^2)^(1/2)
     * @param quat 四元数
     * @return 四元数模长
     */
    inline float length(const Quat quat) noexcept { return std::sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w); }

    /*
     * 为什么要归一化？
     * 只有单位四元数才对应一个纯旋转。四元数在组合、插值、物理积分、
     * 网络反序列化等运算中，浮点误差会让长度缓慢偏离 1：
     * - 长度略大于 1：叠加/插值后旋转出现细微缩放漂移
     * - 长度略小于 1：同理，长帧运行后会肉眼可见地"变形"
     * - 长度恰为 0：除法会直接产生 NaN / Inf，污染整个变换管线
     * 因此这些运算之后都要重新归一化，维持 w^2 + x^2 + y^2 + z^2 = 1 这个不变量
     */
    /**
     * 归一化四元数，把任意非零四元数缩放成"单位四元数"（长度 = 1），使其成为合法的旋转表示。
     *
     * @param quat 四元数
     * @return 单位四元数
     */
    inline Quat normalized(const Quat quat) noexcept
    {
        const float len = length(quat);
        if (len <= 0.0f)
        {
            return Quat::identity();
        }
        const float inv_len = 1.0f / len;
        return {.x = quat.x * inv_len, .y = quat.y * inv_len, .z = quat.z * inv_len, .w = quat.w * inv_len};
    }

    /**
     * 四元数共轭
     * @param quat 四元数
     * @return 共轭四元数
     */
    constexpr Quat conjugate(const Quat quat) noexcept { return {.x = -quat.x, .y = -quat.y, .z = -quat.z, .w = quat.w}; }

    /**
     * 求四元数的逆（单位四元数下等于共轭）
     * @param quat 四元数
     * @return 四元数的逆
     */
    constexpr Quat inverse(const Quat quat) noexcept { return conjugate(quat); }

    /**
     * 轴角 -> 四元数
     * @param axis Vec3 轴
     * @param rad 弧度
     * @note axis 必须是单位向量
     * @return 对应的单位四元数
     */
    inline Quat from_axis_angle(const Vec3 axis, const float rad) noexcept
    {
        const float half = 0.5f * rad;
        const float s = std::sin(half);
        return {.x = axis.x * s, .y = axis.y * s, .z = axis.z * s, .w = std::cos(half)};
    }

    /**
     * 欧拉角 -> 四元数
     * @param rx rx 绕 X 轴的角度（弧度）
     * @param ry ry 绕 Y 轴的角度（弧度）
     * @param rz rz 绕 Z 轴的角度（弧度）
     * @return 组合后的单位四元数
     * @note 复合顺序为 qy * qx * qz，即先绕 Z、再绕 X、最后绕 Y，与入参顺序无关
     */
    inline Quat from_euler_xyz(const float rx, const float ry, const float rz) noexcept
    {
        const Quat qx = from_axis_angle({.x = 1.0f, .y = 0.0f, .z = 0.0f}, rx);
        const Quat qy = from_axis_angle({.x = 0.0f, .y = 1.0f, .z = 0.0f}, ry);
        const Quat qz = from_axis_angle({.x = 0.0f, .y = 0.0f, .z = 1.0f}, rz);
        return qy * qx * qz;
    }

    /**
     * 用四元数旋转一个 3D 向量
     * @param quat 四元数
     * @param vec Vec3 3D向量
     * @return 旋转后的新向量
     */
    inline Vec3 rotate(const Quat quat, const Vec3 vec) noexcept
    {
        const Quat p{.x = vec.x, .y = vec.y, .z = vec.z, .w = 0.0f};
        const Quat r = quat * p * conjugate(quat);
        return {.x = r.x, .y = r.y, .z = r.z};
    }

    /**
     * 把四元数转成 4×4 旋转矩阵
     * @param quat 四元数
     * @return 4x4矩阵
     */
    inline Mat4 to_mat4(const Quat quat) noexcept
    {
        const float xx = quat.x * quat.x;
        const float yy = quat.y * quat.y;
        const float zz = quat.z * quat.z;
        const float xy = quat.x * quat.y;
        const float xz = quat.x * quat.z;
        const float yz = quat.y * quat.z;
        const float wx = quat.w * quat.x;
        const float wy = quat.w * quat.y;
        const float wz = quat.w * quat.z;

        Mat4 r = Mat4::identity();
        r.mat[0][0] = 1.0f - 2.0f * (yy + zz);
        r.mat[0][1] = 2.0f * (xy - wz);
        r.mat[0][2] = 2.0f * (xz + wy);
        r.mat[1][0] = 2.0f * (xy + wz);
        r.mat[1][1] = 1.0f - 2.0f * (xx + zz);
        r.mat[1][2] = 2.0f * (yz - wx);
        r.mat[2][0] = 2.0f * (xz - wy);
        r.mat[2][1] = 2.0f * (yz + wx);
        r.mat[2][2] = 1.0f - 2.0f * (xx + yy);
        return r;
    }

    /**
     * 球面线性插值（SLERP），在两个旋转之间做平滑过渡的核心函数。动画混合、相机平滑、角色转向、网络同步插值全都靠它
     * @param quat_a 起始旋转四元数，t=0 时的结果
     * @param quat_b 目标旋转四元数，t=1 时的结果
     * @param t 插值参数
     * @return 位于 a 与 b 之间、沿球面大圆插值出的单位四元数旋转
     */
    inline Quat slerp(const Quat quat_a, Quat quat_b, const float t) noexcept
    {
        float d = quat_a.x * quat_b.x + quat_a.y * quat_b.y + quat_a.z * quat_b.z + quat_a.w * quat_b.w;
        if (d < 0.0f)
        {
            quat_b = {.x = -quat_b.x, .y = -quat_b.y, .z = -quat_b.z, .w = -quat_b.w};
            d = -d;
        }

        if (d > 0.9995f)
        {
            return normalized(
                Quat{quat_a.x + (quat_b.x - quat_a.x) * t, quat_a.y + (quat_b.y - quat_a.y) * t, quat_a.z + (quat_b.z - quat_a.z) * t, quat_a.w + (quat_b.w - quat_a.w) * t});
        }

        const float theta = std::acos(d);
        const float sin_theta = std::sin(theta);
        const float wa = std::sin((1.0f - t) * theta) / sin_theta;
        const float wb = std::sin(t * theta) / sin_theta;
        return normalized(Quat{quat_a.x * wa + quat_b.x * wb, quat_a.y * wa + quat_b.y * wb, quat_a.z * wa + quat_b.z * wb, quat_a.w * wa + quat_b.w * wb});
    }

} // namespace taixu::core
