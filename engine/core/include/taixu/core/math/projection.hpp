#pragma once

/*
 * 投影与视图矩阵
 *
 * 输出直接符合 Vulkan 约定，不需要额外的坐标翻转：
 * - 裁剪空间深度范围 [0, 1]，近平面 0，远平面 1
 * - NDC 的 Y 轴向下，投影矩阵内部翻转 Y，使世界中的"上"对应屏幕上的"上"
 *
 */

#include <cmath>

#include "taixu/core/math/mat.hpp"
#include "taixu/core/math/vec.hpp"

namespace taixu::core
{
    /**
     * 透视投影，把相机空间（右手系，相机看向 -Z）变换到裁剪空间，模拟“近大远小”
     * @param fov_y_rad 垂直视场角（弧度），即相机上下方向能看到的夹角。越大视野越宽、物体显得越小；常用 45°~60°（约 0.785~1.047 弧度）
     * @param aspect 宽高比，即视口宽度 / 高度。用于水平方向缩放，保证画面不被拉伸；窗口为 16:9 时传 16.0f/9.0f
     * @param near_plane 近裁剪面距离，必须 > 0。比它更近的物体被裁掉；也决定深度精度，太小会导致 z-fighting
     * @param far_plane 远裁剪面距离，必须 > near_plane。比它更远的物体被裁掉
     * @return 4x4 透视投影矩阵。输出 w' = -z（相机空间深度），使用时需做透视除法 (x'/w', y'/w', z'/w')
     * @note （1）near_plane > 0
     * @note （2）far_plane > near_plane
     * @note （3）aspect > 0
     * @note （4）深度是非线性的：near=0.1、far=100 时，5 米处的 z_ndc 已经是 0.98，精度几乎全被近处吃掉，这是后面反向 Z / 对数深度要解决的问题
     */
    inline Mat4 perspective(const float fov_y_rad, const float aspect, const float near_plane, const float far_plane) noexcept
    {
        // f 是垂直视场角一半的余切值，决定“视野有多宽”。fov_y_rad 越大，f 越小，物体看起来越小
        // fov
        const float f = 1.0f / std::tan(0.5f * fov_y_rad);

        Mat4 r;
        r.mat[0][0] = f / aspect;
        r.mat[1][1] = -f; // 负号：NDC 的 Y 向下
        r.mat[2][2] = far_plane / (near_plane - far_plane);
        r.mat[2][3] = far_plane * near_plane / (near_plane - far_plane);
        // 透视除法的开关：令 w' = -z_view
        // 负号的原因：相机看向 -Z，前方的点 z_view < 0，取负才得到正的距离
        // 硬件在矩阵之后做 (x'/w', y'/w', z'/w')，这一步就是“近大远小”的来源
        r.mat[3][2] = -1.0f;
        return r;
    }

    /**
     * 正交投影矩阵构造，把相机空间的一个长方体视景体（由 left/right/bottom/top/near/far 定义）映射到 NDC 的 [-1,1] × [-1,1] × [0,1] 长方体
     * @param left 视景体在 X 方向的左边界
     * @param right 视景体在 X 方向的右边界
     * @param bottom 视景体在 Y 方向的下边界
     * @param top 视景体在 Y 方向的上边界
     * @param near_plane 近裁剪面到相机的距离
     * @param far_plane 远裁剪面到相机的距离
     * @return 4x4 正交投影矩阵，输出 w' 恒为 1，无需透视除法
     * @note （1）right > left
     * @note （2）top > bottom
     * @note （3）far_plane > near_plane
     */
    inline Mat4 orthographic(const float left, const float right, const float bottom, const float top, const float near_plane,
                             const float far_plane) noexcept
    {
        Mat4 r = Mat4::identity();
        r.mat[0][0] = 2.0f / (right - left);
        r.mat[0][3] = -(right + left) / (right - left);
        r.mat[1][1] = -2.0f / (top - bottom); // 负号：NDC 的 Y 向下
        r.mat[1][3] = (top + bottom) / (top - bottom);
        r.mat[2][2] = -1.0f / (far_plane - near_plane);
        r.mat[2][3] = -near_plane / (far_plane - near_plane);
        return r;
    }

    /*
     * 视图矩阵 = 在相机基底下重新表示，再把眼睛挪到原点
     * 一个向量在新基底下的坐标，就是它在各轴上的投影
     * 视图矩阵等价于“把整个世界反向搬动，让相机回到原点、朝向 -Z”
     */
    /**
     * 视图矩阵：把世界空间变换到相机空间
     * @note （1）右手系，相机看向 -Z，up 只需与视线不平行
     * @note （2）up 与视线平行时 cross(f, up) 为零向量，normalized 返回零向量，矩阵退化
     * @note （3）第 3 行是 (-f, +dot(f, eye))，前两行是 (s, -dot(s, eye))，符号不同是因为相机的第三条基向量是 -f 而不是 f
     * @param eye Vec3 相机位置
     * @param target Vec3 看向的点
     * @param up Vec3 上方向
     * @return 4x4矩阵
     */
    inline Mat4 look_at(const Vec3 eye, const Vec3 target, const Vec3 up) noexcept
    {
        const Vec3 f = normalized(target - eye);
        const Vec3 s = normalized(cross(f, up));
        const Vec3 u = cross(s, f);

        Mat4 r = Mat4::identity();
        r.mat[0][0] = s.x;
        r.mat[0][1] = s.y;
        r.mat[0][2] = s.z;
        r.mat[0][3] = -dot(s, eye);
        r.mat[1][0] = u.x;
        r.mat[1][1] = u.y;
        r.mat[1][2] = u.z;
        r.mat[1][3] = -dot(u, eye);
        r.mat[2][0] = -f.x;
        r.mat[2][1] = -f.y;
        r.mat[2][2] = -f.z;
        r.mat[2][3] = dot(f, eye);
        return r;
    }

} // namespace taixu::core
