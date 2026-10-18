#pragma once

/*
 * 4x4 矩阵
 *
 * 存储为行主序：mat[row][col]，mat[r][c] 的内存偏移为 r * 4 + c
 * 变换按列向量右乘：v' = M * v
 * 平移分量在第四列：mat[0][3] / mat[1][3] / mat[2][3]
 *
 * 上传给 GPU 时注意：HLSL 常量缓冲默认按列主序打包，需要转置或声明 row_major
 *
 */

#include <cmath>

#include "taixu/core/math/vec.hpp"

namespace taixu::core
{
    /*
     * 【背景】什么是4x4矩阵？干什么？怎么用？
     * 4x4矩阵就是字面意思，一个四行四列的矩阵
     *
     * 4x4矩阵是 3D 图形管线中“几何变换”这一环最重要的工具，它能做到的事情比如：
     * 平移、旋转、缩放、镜像/反射、错切/剪切。
     * 在3D的世界中，想要把一个点向量（3D中一个点要用三个坐标来表示，比如x,y,z）执行以上操作，无疑需要将它的x,y,z全都算一次
     * 一两次运算还行，手写几次也无伤大雅，但是一旦量变引起质变，算个 1*10^5次的那你不就van dan了
     * 因此4x4 矩阵的价值不在单次运算更快，而在变换可以预先复合：
     * 多个变换先乘成一个矩阵，再作用于任意多顶点
     *
     * 那问题来了，一个 3D向量 做变化为什么要乘以 4x4的矩阵，而不是乘以 3x3矩阵？这是因为：
     * 3D 空间里的点坐标是 (x, y, z)，而3×3 矩阵表示 R^3 上的线性映射，线性映射又必然把原点映到原点（恒有 M(0)=0）。平移把原点挪到别处，所以平移不是线性的，无法用 3×3 在 R^3 上表示，
     * 而齐次坐标（下面会说到）的做法的本质是：升一维，让平移在新的高维空间里变成线性映射
     * （↑此处建议至少高中文凭阅读，初中的bro就问一下DeepSeek吧）
     * 解决办法是给点补一个分量变成 (x, y, z, 1)，也就是 Vec4，用 4x4 矩阵统一表示：
     * [ 1 0 0 tx]
     * [ 0 1 0 ty]
     * [ 0 0 1 tz]
     * [ 0 0 0 1 ]
     * 这叫齐次坐标（homogeneous coordinates）
     * 第 4 列（或第 4 行，看约定，Taixu这里用的是第四列）放平移量，前 3x3 放旋转/缩放。这样一来，旋转、缩放、平移都能用同一种运算（矩阵乘法）串起来
     *
     * 简单给不了解矩阵运算的bro讲一下：一个 4x4矩阵乘以一个 4D向量（[x, y, z, w]）是这样算的：
     *
     * 假设矩阵M为：
     * [m00 m01 m02 m03]
     * [m10 m11 m12 m13]
     * [m20 m21 m22 m23]
     * [m30 m31 m32 m33]
     *
     * 4D向量s为：[x, y, z, w]
     *
     * 则 M*s = [x', y', z' ,w'] 的计算为：
     * x' = m00*x + m01*y + m02*z + m03*w
     * y' = m10*x + m11*y + m12*z + m13*w
     * z' = m20*x + m21*y + m22*z + m23*w
     * w' = m30*x + m31*y + m32*z + m33*w
     *
     * 也就是结果的第 i 个分量 = 矩阵第 i 行与向量的点积
     *
     * 关于这个 w 是什么：
     * 4x4 矩阵计算里的 w，是齐次坐标的第 4 个分量
     * 它本身不是“第 4 维空间”，而是一个辅助量，承担两个关键作用：区分点和方向、以及实现透视投影
     *
     * 【1】输入时：w 决定“这是点还是方向”：
     * 当你把一个 3D向量补成 4维时：
     * - w = 1 表示原本的 3D向量是一个点（位置），此时进行矩阵运算时，矩阵的平移量会作用上去
     * - w = 0 表示原本的 3D向量是一个方向（法线、速度），此时进行矩阵运算时，平移量被乘 0 屏蔽，只受旋转/缩放影响
     *
     * 【2】输出时：w 决定“要不要做透视除法”：
     * 矩阵乘完后得到 (x', y', z', w')。这个输出的 w' 才是图形学里真正重要的东西
     * （1）仿射变换（平移/旋转/缩放/镜像）：
     *     输入 w = 1，输出 w' 绝对还是 1 => 直接取 (x', y', z')，不需要除法
     * （2）透视投影：
     *     透视投影矩阵会故意让 w' 不再等于 1，而是等于相机空间下的深度（通常是 -z 或 z 相关量）。这时必须做透视除法：
     *     屏幕坐标 = (x'/w', y'/w', z'/w')
     *     ！！！这一步就是“近大远小”的来源：离相机越远，w' 越大，x'/w'、y'/w' 就被压得越小，物体在屏幕上显得越小
     *     注意 z' 也被同一个 w' 除掉，所以深度的分布在近处密、远处疏，
     *     这是后面深度精度问题的来源
     *
     * ---
     * 矩阵乘矩阵 = 映射的复合。A*B 作用到向量时 B 先发生
     * 所以 T*R*S 表示"先缩放、再旋转、最后平移"，也是 T*R ≠ R*T 的原因
     */
    /**
     * 4x4矩阵结构体
     */
    struct Mat4
    {
        float mat[4][4]{};

        /**
         * 访问4x4矩阵中的某一行（非const）
         * @param row 4x4矩阵的行索引
         * @return 指向该行首个元素的指针，配合 [col] 使用
         */
        constexpr float* operator[](const int row) noexcept { return mat[row]; }

        /**
         * 访问4x4矩阵中的某一行（const）
         * @param row 4x4矩阵的行索引
         * @return 指向该行首个元素的指针，配合 [col] 使用
         */
        constexpr const float* operator[](const int row) const noexcept { return mat[row]; }

        /**
         * 生成并返回一个4x4单位矩阵
         * @note 单位矩阵是矩阵里的“1”——任何矩阵乘它，结果都不变
         * @return 4x4单位矩阵
         */
        static constexpr Mat4 identity() noexcept
        {
            Mat4 r;
            r.mat[0][0] = 1.0f;
            r.mat[1][1] = 1.0f;
            r.mat[2][2] = 1.0f;
            r.mat[3][3] = 1.0f;
            return r;
        }

        /**
         * 生成一个平移矩阵，把点沿 X、Y、Z 方向分别移动 t.x、t.y、t.z
         * @param t 平移向量
         * @note 输入点的 w 必须是 1
         * @return 平移矩阵
         */
        static constexpr Mat4 translation(const Vec3 t) noexcept
        {
            Mat4 r = identity();
            r.mat[0][3] = t.x;
            r.mat[1][3] = t.y;
            r.mat[2][3] = t.z;
            return r;
        }

        /**
         * 生成一个缩放矩阵，把点沿 X、Y、Z 轴分别乘以 s.x、s.y、s.z
         * @param s 缩放系数向量
         * @return 缩放矩阵
         */
        static constexpr Mat4 scaling(const Vec3 s) noexcept
        {
            Mat4 r;
            r.mat[0][0] = s.x;
            r.mat[1][1] = s.y;
            r.mat[2][2] = s.z;
            r.mat[3][3] = 1.0f;
            return r;
        }

        /**
         * 生成绕 X 轴旋转的4x4矩阵
         * @param rad 弧度
         * @return 绕 X 轴旋转的4x4矩阵
         * @note 正方向由右手定则给出：右手拇指指向 +X，四指弯曲的方向即正旋转方向
         */
        static Mat4 rotation_x(const float rad) noexcept
        {
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            Mat4 r = identity();
            r.mat[1][1] = c;
            r.mat[1][2] = -s;
            r.mat[2][1] = s;
            r.mat[2][2] = c;
            return r;
        }

        /**
         * 生成绕 Y 轴旋转的4x4矩阵
         * @param rad 弧度
         * @return 绕 Y 轴旋转的4x4矩阵
         * @note 正方向由右手定则给出：右手拇指指向 +Y，四指弯曲的方向即正旋转方向
         */
        static Mat4 rotation_y(const float rad) noexcept
        {
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            Mat4 r = identity();
            r.mat[0][0] = c;
            r.mat[0][2] = s;
            r.mat[2][0] = -s;
            r.mat[2][2] = c;
            return r;
        }

        /**
         * 生成绕 Z 轴旋转的4x4矩阵
         * @param rad 弧度
         * @return 绕 Z 轴旋转的4x4矩阵
         * @note 正方向由右手定则给出：右手拇指指向 +Z，四指弯曲的方向即正旋转方向
         */
        static Mat4 rotation_z(const float rad) noexcept
        {
            const float c = std::cos(rad);
            const float s = std::sin(rad);
            Mat4 r = identity();
            r.mat[0][0] = c;
            r.mat[0][1] = -s;
            r.mat[1][0] = s;
            r.mat[1][1] = c;
            return r;
        }
    };

    /**
     * 两个 4x4 矩阵相乘，标准的矩阵乘法 r = mat_a * mat_b
     * @param mat_a 4x4 矩阵
     * @param mat_b 4x4 矩阵
     * @return 新的 4x4 矩阵，即 mat_a * mat_b 的乘积结果
     */
    constexpr Mat4 operator*(const Mat4& mat_a, const Mat4& mat_b) noexcept
    {
        Mat4 r;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                r.mat[i][j] =
                    mat_a.mat[i][0] * mat_b.mat[0][j] + mat_a.mat[i][1] * mat_b.mat[1][j] + mat_a.mat[i][2] * mat_b.mat[2][j] + mat_a.mat[i][3] * mat_b.mat[3][j];
            }
        }
        return r;
    }

    /**
     * 4x4矩阵 乘 Vec4向量
     * @param mat 4x4矩阵
     * @param vec Vec4向量
     * @return 变换后的 Vec4向量
     */
    constexpr Vec4 operator*(const Mat4& mat, const Vec4 vec) noexcept
    {
        return {.x = mat.mat[0][0] * vec.x + mat.mat[0][1] * vec.y + mat.mat[0][2] * vec.z + mat.mat[0][3] * vec.w,
                .y = mat.mat[1][0] * vec.x + mat.mat[1][1] * vec.y + mat.mat[1][2] * vec.z + mat.mat[1][3] * vec.w,
                .z = mat.mat[2][0] * vec.x + mat.mat[2][1] * vec.y + mat.mat[2][2] * vec.z + mat.mat[2][3] * vec.w,
                .w = mat.mat[3][0] * vec.x + mat.mat[3][1] * vec.y + mat.mat[3][2] * vec.z + mat.mat[3][3] * vec.w};
    }

    /**
     * 把 3D点用一个 4x4 矩阵做变换
     * @note 点：w 分量取 1，不做透视除法，投影矩阵的结果要用 Vec4 版本再加除法
     * @param mat 4x4矩阵
     * @param point Vec3 3D点
     * @return 变换后的 Vec3 3D 点
     */
    constexpr Vec3 transform_point(const Mat4& mat, const Vec3 point) noexcept
    {
        const Vec4 r = mat * Vec4{.x = point.x, .y = point.y, .z = point.z, .w = 1.0f};
        return {.x = r.x, .y = r.y, .z = r.z};
    }

    /**
     * 把 3D方向（法线、速度、朝向等）用一个 4x4 矩阵做变换
     * @note 方向：w 分量取 0，平移被屏蔽，只受旋转与缩放影响
     * @param mat 4x4矩阵
     * @param direction Vec3 3D 方向
     * @return 变换后的 Vec3 3D 方向
     */
    constexpr Vec3 transform_direction(const Mat4& mat, const Vec3 direction) noexcept
    {
        const Vec4 r = mat * Vec4{.x = direction.x, .y = direction.y, .z = direction.z, .w = 0.0f};
        return {.x = r.x, .y = r.y, .z = r.z};
    }

    /**
     * 矩阵转置：把矩阵的行和列互换，返回
     * @param mat 4x4矩阵
     * @return 转置后的新矩阵
     * @note （1）对旋转矩阵，转置等于求逆（正交矩阵满足 Mᵀ * M = I）
     * @note （2）非等比缩放下的法线变换需要用到它，见 transform.hpp 的 normal_matrix()
     */
    constexpr Mat4 transposed(const Mat4& mat) noexcept
    {
        Mat4 r;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                r.mat[i][j] = mat.mat[j][i];
            }
        }
        return r;
    }

    /**
     * 4x4矩阵求逆：给定矩阵 mat，返回 mat的逆矩阵，使得 mat * mat^-1 = mat^-1 * mat = I（单位矩阵）
     * @param mat 4x4矩阵
     * @note 奇异矩阵返回全零
     * @return mat的逆矩阵
     */
    inline Mat4 inverse(const Mat4& mat) noexcept
    {
        const float* s = &mat.mat[0][0];
        float inv[16];

        inv[0] = s[5] * s[10] * s[15] - s[5] * s[11] * s[14] - s[9] * s[6] * s[15] + s[9] * s[7] * s[14] +
            s[13] * s[6] * s[11] - s[13] * s[7] * s[10];
        inv[4] = -s[4] * s[10] * s[15] + s[4] * s[11] * s[14] + s[8] * s[6] * s[15] - s[8] * s[7] * s[14] -
            s[12] * s[6] * s[11] + s[12] * s[7] * s[10];
        inv[8] = s[4] * s[9] * s[15] - s[4] * s[11] * s[13] - s[8] * s[5] * s[15] + s[8] * s[7] * s[13] +
            s[12] * s[5] * s[11] - s[12] * s[7] * s[9];
        inv[12] = -s[4] * s[9] * s[14] + s[4] * s[10] * s[13] + s[8] * s[5] * s[14] - s[8] * s[6] * s[13] -
            s[12] * s[5] * s[10] + s[12] * s[6] * s[9];
        inv[1] = -s[1] * s[10] * s[15] + s[1] * s[11] * s[14] + s[9] * s[2] * s[15] - s[9] * s[3] * s[14] -
            s[13] * s[2] * s[11] + s[13] * s[3] * s[10];
        inv[5] = s[0] * s[10] * s[15] - s[0] * s[11] * s[14] - s[8] * s[2] * s[15] + s[8] * s[3] * s[14] +
            s[12] * s[2] * s[11] - s[12] * s[3] * s[10];
        inv[9] = -s[0] * s[9] * s[15] + s[0] * s[11] * s[13] + s[8] * s[1] * s[15] - s[8] * s[3] * s[13] -
            s[12] * s[1] * s[11] + s[12] * s[3] * s[9];
        inv[13] = s[0] * s[9] * s[14] - s[0] * s[10] * s[13] - s[8] * s[1] * s[14] + s[8] * s[2] * s[13] +
            s[12] * s[1] * s[10] - s[12] * s[2] * s[9];
        inv[2] = s[1] * s[6] * s[15] - s[1] * s[7] * s[14] - s[5] * s[2] * s[15] + s[5] * s[3] * s[14] +
            s[13] * s[2] * s[7] - s[13] * s[3] * s[6];
        inv[6] = -s[0] * s[6] * s[15] + s[0] * s[7] * s[14] + s[4] * s[2] * s[15] - s[4] * s[3] * s[14] -
            s[12] * s[2] * s[7] + s[12] * s[3] * s[6];
        inv[10] = s[0] * s[5] * s[15] - s[0] * s[7] * s[13] - s[4] * s[1] * s[15] + s[4] * s[3] * s[13] +
            s[12] * s[1] * s[7] - s[12] * s[3] * s[5];
        inv[14] = -s[0] * s[5] * s[14] + s[0] * s[6] * s[13] + s[4] * s[1] * s[14] - s[4] * s[2] * s[13] -
            s[12] * s[1] * s[6] + s[12] * s[2] * s[5];
        inv[3] = -s[1] * s[6] * s[11] + s[1] * s[7] * s[10] + s[5] * s[2] * s[11] - s[5] * s[3] * s[10] -
            s[9] * s[2] * s[7] + s[9] * s[3] * s[6];
        inv[7] = s[0] * s[6] * s[11] - s[0] * s[7] * s[10] - s[4] * s[2] * s[11] + s[4] * s[3] * s[10] +
            s[8] * s[2] * s[7] - s[8] * s[3] * s[6];
        inv[11] = -s[0] * s[5] * s[11] + s[0] * s[7] * s[9] + s[4] * s[1] * s[11] - s[4] * s[3] * s[9] -
            s[8] * s[1] * s[7] + s[8] * s[3] * s[5];
        inv[15] = s[0] * s[5] * s[10] - s[0] * s[6] * s[9] - s[4] * s[1] * s[10] + s[4] * s[2] * s[9] +
            s[8] * s[1] * s[6] - s[8] * s[2] * s[5];

        const float det = s[0] * inv[0] + s[1] * inv[4] + s[2] * inv[8] + s[3] * inv[12];
        if (det == 0.0f)
        {
            return Mat4{};
        }

        const float inv_det = 1.0f / det;
        Mat4 r;
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                r.mat[i][j] = inv[i * 4 + j] * inv_det;
            }
        }
        return r;
    }

} // namespace taixu::core
