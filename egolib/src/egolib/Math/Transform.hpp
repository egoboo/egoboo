#pragma once

#include "egolib/Math/Standard.hpp"

namespace Ego {
namespace Math {

struct Transform {
    /**
     * @brief
     *  Get a viewing transformation (~ world space -> camera space) matrix.
     * @param eye
     *  the position of the eye point
     * @param center
     *  the position of the reference point
     * @param up
     *  the direction of the up vector
     * @return
     *  the matrix
     * @pre
     *  eye != center (debug & release)
     *  up  != 0
     */
    static Matrix4f4f lookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up)
    {
        Vector3f f = center - eye;
        Vector3f u = up;

        f.normalize();
        u.normalize();

        Vector3f s = f.cross(u);
        s.normalize();

        u = s.cross(f);

        return
            Matrix4f4f
            (
                 s[kX],  s[kY],  s[kZ], 0.0f,
                 u[kX],  u[kY],  u[kZ], 0.0f,
                -f[kX], -f[kY], -f[kZ], 0.0f,
                  0.0f,   0.0f,   0.0f, 1.0f
            )
            *
            Transform::translation(-eye);
    }

    /**
     * @brief
     *  Get an orthographic projection (~ camera space -> normalized device coordinate space) matrix.
     * @param left, right
     *  the coordinates of the left and right vertical clipping planes
     * @param bottom, top
     *  the coordinates of the bottom and top horizontal clipping planes
     * @param zNear, zFar
     *  the distance to the nearer and farther depth clipping planes.
     *  These values are negative if the plane is to be behind the viewer.
     * @return
     *  the matrix
     * @remark
     *  The orthographic projection matrix is
     *  \f[
     *  \left[\begin{matrix}
     *  \frac{2}{right-left} & 0                    &   0                    & t_x \\
     *  0                    & \frac{2}{top-bottom} &   0                    & t_y \\
     *  0                    & 0                    &  \frac{-2}{zFar-zNear} & t_z \\
     *  0                    & 0                    &   0                    & 1 \\
     *  \end{matrix}\right]
     *  \f]
     *  where \f$t_x = -\frac{right+left}{right-left}\f$,
     *      \f$t_y = -\frac{top+bottom}{top-bottom}\f$,
     *      \f$t_z = -\frac{zFar+zNear}{zFar-zNear}\f$.
     */
    static Matrix4f4f ortho(const float left, const float right, const float bottom, const float top, const float zNear, const float zFar) {
        float dx = right - left, dy = top - bottom, dz = zFar - zNear;
        EGOBOO_ASSERT(dx != 0.0f && dy != 0.0f && dz != 0.0f);
        float tx = -(right + left) / dx, ty = -(top + bottom) / dy, tz = -(zFar + zNear) / (dz);

        return
            Matrix4f4f
            (
                2.0f / dx, 0.0f,     0.0f,    tx,
                0.0f,      2.0f/dy,  0.0f,    ty,
                0.0f,      0.0f,    -2.0f/dz, tz,
                0.0f,      0.0f,     0.0f,    1.0f
            );
    }

    /**@{*/

    /**
     * @brief
     *  Get a perspective projection (~ view space -> normalized device coordinate space) matrix.
     * @param fovy
     *  the field of view angle in the y direction
     * @param aspect
     *  the aspect ratio in the x direction
     * @param zNear
     *  the distance of the viewer to the near clipping plane
     * @param zFrar
     *  the distance of the viewer to the far clipping plane
     * @return
     *  the matrix
     * @pre
     *  @a zNear as well as @a zFar must be positive, <tt>zNear - zFar</tt> must not be @a 0,
     *  @a aspect must not be @a 0.
     * @remark
     *  The aspect ratio specifies the field of view in the x direction and is the ratio of the x (width) / y (height).
     * @remark
     *  The perspective projection matrix is
     *  \f[
     *  \left[\begin{matrix}
     *  \frac{f}{aspect} & 0 &  0                                     & 0 \\
     *  0                & f &  0                                     & 0 \\
     *  0                & 0 &  \frac{(zFar + zNear)}{(zNear - zFar)} & \frac{(2 * zFar * zNear)}{(zNear - zFar)} \\
     *  0                & 0 & -1                                     & 1 \\
     *  \end{matrix}\right]
     *  \f]
     *  where \f$f = cot(0.5 fovy)\f$.
     */
    static Matrix4f4f perspective(const Ego::Math::Degrees& fovy, const float aspect, const float zNear, const float zFar) {
        return perspective(Ego::Math::Radians(fovy), aspect, zNear, zFar);
    }

    static Matrix4f4f perspestive(const Ego::Math::Turns& fovy, const float aspect, const float zNear, const float zFar) {
        return perspective(Ego::Math::Radians(fovy), aspect, zNear, zFar);
    }

    static Matrix4f4f perspective(const Ego::Math::Radians& fovy, const float aspect, const float zNear, const float zFar) {
        EGOBOO_ASSERT(aspect != 0.0f);
        EGOBOO_ASSERT(zFar > 0.0f && zNear > 0.0f);
        EGOBOO_ASSERT((zNear - zFar) != 0.0f);

        float tan = std::tan(fovy * 0.5f);
        EGOBOO_ASSERT(tan != 0.0f);
        float f = 1 / tan;

        return
            Matrix4f4f
            (
                f / aspect, 0.0f, 0.0f,                            0.0f,
                0.0f,       f,    0.0f,                            0.0f,
                0.0f,       0.0f, (zFar + zNear) / (zNear - zFar), (2.0f * zFar * zNear) / (zNear - zFar),
                0.0f,       0.0f, -1.0f,                           1.0f
            );
    }

    /**@}*/

    /**
     * @brief
     *  Assign this matrix the values of a translation matrix.
     * @param t
     *  the translation vector
     * @remark
     *  The \f$4 \times 4\f$ translation matrix for the translation vector $\left(t_x,t_y,t_z\right)$ is defined as
     *  \f[
     *  \left[\begin{matrix}
     *  1 & 0 & 0 & t_x \\
     *  0 & 1 & 0 & t_y \\
     *  0 & 0 & 1 & t_z \\
     *  0 & 0 & 0 & 1   \\
     *  \end{matrix}\right]
     *  \f]
     */
    static Matrix4f4f translation(const Vector3f& t) {
        return
            Matrix4f4f
            (
                1, 0, 0, t.x(),
                0, 1, 0, t.y(),
                0, 0, 1, t.z(),
                0, 0, 0,   1
            );
    }

    /**@{*/

    /**
     * @brief
     *  Get a matrix representing an anticlockwise rotation around the x-axis.
     * @param a
     *  the angle of rotation
     * @return
     *  the matrix
     * @remark
     *  \f[
     *  \left[\begin{matrix}
     *  1 & 0 &  0 & 0 \\
     *  0 & c & -s & 0 \\
     *  0 & s &  c & 0 \\
     *  0 & 0 &  0 & 1 \\
     *  \end{matrix}\right]
     *  \f]
     *  where \f$c=\cos(a)\f$ and \f$s=\sin(a)\f$.
     */
    static Matrix4f4f rotationX(const Ego::Math::Degrees& a) {
        return rotationX(Ego::Math::Radians(a));
    }

    static Matrix4f4f rotationX(const Ego::Math::Turns& a) {
        return rotationX(Ego::Math::Radians(a));
    }

    static Matrix4f4f rotationX(const Ego::Math::Radians& a) {
        float c = std::cos(a), s = std::sin(a);
        return
            Matrix4f4f
            (
                1,  0,  0, 0,
                0, +c, -s, 0,
                0, +s,  c, 0,
                0,  0,  0, 1
            );
    }

    /**@}*/

    /**@{*/

    /**
     * @brief
     *  Get a matrix representing a anticlockwise rotation around the y-axis.
     * @param a
     *  the angle of rotation
     * @return
     *  the matrix
     * @remark
     *  \f[
     *  \left[\begin{matrix}
     *   c & 0 & s & 0 \\
     *   0 & 1 & 0 & 0 \\
     *  -s & 0 & c & 0 \\
     *   0 & 0 & 0 & 1 \\
     *  \end{matrix}\right]
     *  \f]
     *  where \f$c=\cos(a)\f$ and \f$s=\sin(a)\f$.
     */
    static Matrix4f4f rotationY(const Ego::Math::Degrees& a) {
        return rotationY(Ego::Math::Radians(a));
    }

    static Matrix4f4f rotationY(const Ego::Math::Turns& a) {
        return rotationY(Ego::Math::Turns(a));
    }

    static Matrix4f4f rotationY(const Ego::Math::Radians& a) {
        float c = std::cos(a), s = std::sin(a);
        return
            Matrix4f4f
            (
                +c, 0, +s, 0,
                 0, 1,  0, 0,
                -s, 0, +c, 0,
                 0, 0,  0, 1
            );
    }

    /**@}*/

    /**@{*/

    /**
     * @brief
     *  Get a matrix representing an anticlockwise rotation about the z-axis.
     * @param a
     *  the angle of rotation
     * @return
     *  the matrix
     * @remark
     *  \f[
     *  \left[\begin{matrix}
     *  c & -s & 0 & 0 \\
     *  s &  c & 0 & 0 \\
     *  0 &  0 & 1 & 0 \\
     *  0 &  0 & 0 & 1 \\
     *  \end{matrix}\right]
     *  \f]
     *  where \f$c=\cos(a)\f$ and \f$s=\sin(a)\f$.
     */
    static Matrix4f4f rotationZ(const Ego::Math::Degrees& a) {
        return rotationZ(Ego::Math::Radians(a));

    }

    static Matrix4f4f rotationZ(const Ego::Math::Turns& a) {
        return rotationZ(Ego::Math::Radians(a));
    }

    static Matrix4f4f rotationZ(const Ego::Math::Radians& a) {
        float c = std::cos(a), s = std::sin(a);
        return
            Matrix4f4f
            (
                +c, -s, 0, 0,
                +s, +c, 0, 0,
                 0,  0, 1, 0,
                 0,  0, 0, 1
            );
    }

    /**@}*/


    /**@{*/

    /**
     * @brief
     *  Get a rotation matrix representing a counter-clockwise rotation around an axis.
     * @param axis
     *  the rotation axis
     * @param angle
     *  the rotation angle
     * @return
     *  the rotation matrix
     * @throw std::invalid_argument
     *  if the rotation axis is the zero vector
     * @remark
     *  Given an axis of rotation represented by the unit vector \f$\hat{r}=(k_x,k_y,k_z,1)\f$ and an angle
     *  \f$\theta\f$, we shall obtain a \f$4 \times 4\f$ matrix \f$\mathcal{T}\f$ called the Rodrigues rotation
     *  matrix \f$R\f$ for the axis of rotation \f$\hat{k}\f$ and the angle \f$\theta\f$. This matrix has
     *  represents the transformation of rotating counter-clockwise by \f$\theta\f$ about the axis
     *  \f$\hat{k}\f$ i.e. for any point \f$\vec{v}=(v_x,v_y,v_z,1)\f$
     *  \f[
     *  \vec{v}' = \mathbf{L} \vec{v}
     *  \f]
     *  is the counter-clockwise rotation of \f$\vec{v}\f$ by \f$\theta\f$ about the axis \f$\hat{\vec{r}}\f$.
     *  </br>
     *  The derivation of that matrix is provided here for reference, the geometric reasoning is omitted.
     *  </br>
     *  To compute the rotated point \f$\vec{v}'\f$, we begin by splutting \f$\vec{v}\f$ into a part \f$\vec{v}_{\parallel}\f$
     *  parallel to \f$\hat{\vec{r}}\f$ and into a part \f$\vec{v}_{\perp}\f$ perpendicular to \f$\hat{\vec{r}}\f$ which lies
     *  on the plane of rotation. Recall from Vector3f::decompose(const Vector3f&,const Vector3f&,Vector3f&,Vector3f&) that the
     *  parallel part is the projection of \f$\vec{v}\f$ on \f$\hat{\vec{r}}\f$, the perpendicular part is the rejection
     *  \f$\vec{v}_{\perp}\f$ of \f$\vec{v}\f$ from \f$\hat{\vec{r}}\f$:
     *  \f{align*}{
     *  \vec{v}_{\parallel} = \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\\
     *  \vec{v}_{\perp} = \vec{v} - \vec{v}_{\parallel}
     *  \f}
     *  To compute the effect of the rotation, a two-dimensional basis on the plane of rotation is required.
     *  \f$\vec{v}_{\perp}\f$ is used as the first basis vector. As second basis vector perpendicular to
     *  \f$\vec{v}_{\perp}\f$ is required. One can use the vector
     *  \f{align*}{
     *  \vec{w} = \hat{\vec{r}} \times \vec{v}_{\perp} = \hat{\vec{r}} \times \vec{v}
     *  \f}
     *  which is perpendicular to and has the same length as \f$\vec{v}_{\perp}\f$ as
     *  shown in Vector3f::decompose(const Vector3f&,const Vector3f&,Vector3f&,Vector3f&).
     *  </br>
     *  If in \f$\mathbb{R}^2\f$ one rotates the vector \f$\vec{i}=(1,0)\f$ by \f$\theta\f$
     *  in the plane of rotation spanned by the standard basis \f$\vec{i}=(1,0)\f$,
     *  \f$\vec{j}=(0,1)\f$ for \f$\mathbb{R}^2\f$, the result is the vector
     *  \f{align*}{
     *  \vec{i}' = \cos(\theta)\vec{i} + \sin(\theta) \vec{j}
     *  \f}
     *  If \f$\vec{v}_{\perp}\f$ and \f$\vec{w}\f$ are used as the standard basis and
     *  \f$\vec{v}_{\perp}\f$ is rotated in the plane of rotation spanned by the basis
     *  \f$\vec{v}_{\perp}\f$ and \f$\vec{w}\f$, the result of the rotation is given in
     *  a similar manner:
     *  \f{align*}{
     *  \vec{v}'_{\perp} = \cos(\theta)\vec{v}_{\perp} + \sin(\theta)\vec{w}
     *  \f}
     *  </br>
     *  As \f$\vec{v}_{\parallel}\f$ is completely unaffected by the rotation about \f$\hat{\vec{r}}\f$
     *  the final result of the anti-clockwise rotation of \f$\vec{v}\f$ by \f$\theta\f$ degree around
     *  \f$\hat{\vec{r}}\f$ is given by
     *  \f[
     *  \vec{v}' = \vec{v}_{\parallel} + \vec{v}'_{\perp}
     *  \f]
     *  </br>
     *  As
     *  \f{align*}{
     *  \vec{v}_{\parallel} =& \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\\
     *  \vec{v}_{\perp}     =& \vec{v} - \vec{v}_{\parallel} = \vec{v}_{\parallel}\\
     *                    =& \vec{v} - \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\\
     *  \vec{w}             =& \hat{\vec{r}} \times \vec{v}\\
     *  \vec{v}'_{\perp}    =& \cos(\theta)\vec{v}_{\perp} + \sin(\theta)\vec{w}\\
     *                    =& \cos(\theta)\left(\vec{v} - \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\right) + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)
     *  \f}
     *  the above expression can be rewritten and simplified
     *  \f{align*}{
     *  \vec{v}' =& \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \cos(\theta)\left(\vec{v} - \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\right) + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)\\
     *         =& \left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \cos(\theta)\vec{v} - \cos(\theta)\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)\\
     *         =& \cos(\theta)\vec{v} + \left[\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} - \cos(\theta)\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}}\right] + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)\\
     *         =& \cos(\theta)\vec{v} + (1- \cos(\theta))\left(\vec{v} \cdot \hat{\vec{r}}\right) \hat{\vec{r}} + \sin(\theta)\left(\hat{\vec{r}} \times \vec{v}\right)
     *  \f}
     *  which is known as the Rodrigues rotation formula.
     * @remark
     *  To obtain the Rodrigues rotation formula in a matrix form, the projection \f$\left(\vec{v} \cdot \hat{\vec{r}}
     *  \right)\vec{r}\f$ can be replaced by the tensor product \f$(\hat{\vec{r}} \otimes \hat{\vec{r}})\vec{v}\f$
     *  and the cross product \f$\hat{\vec{r}} \times \vec{v}\f$ can be replaced by a multiplication with a cross product
     *   matrix \f$\mathbf{R} \vec{v}\f$ which gives
     *  \f{align*}{
     *  \vec{v}' =& \cos(\theta)\vec{v} + (1- \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right)\vec{v} + \sin(\theta)\left(\mathbf{R}\vec{v}\right)\\
     *         =& \left[\cos(\theta)\mathbf{I} + (1 - \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + \sin(\theta)\mathbf{R}\right]\vec{v}
     *  \f}
     *  with
     *  \f[
     *  \mathbf{R} =
     *  \left[\begin{matrix}
     *   0                  & -\hat{\vec{r}}_3 &  \hat{\vec{r}}_2 \\
     *   \hat{\vec{r}}_3    & 0                & -\hat{\vec{r}}_1 \\
     *  -\hat{\vec{r}}_2    & \hat{\vec{r}}_1  & 0
     *  \end{matrix}\right]
     *  \f]
     *  and
     *  \f[
     *  \hat{\vec{r}} \otimes \hat{\vec{r}}
     *  =
     *  \left[\begin{matrix}
     *   \hat{\vec{r}}_1 \\
     *   \hat{\vec{r}}_2 \\
     *   \hat{\vec{r}}_3
     *  \end{matrix}\right]
     *  \otimes
     *  \left[\begin{matrix}
     *   \hat{\vec{r}}_1 & \hat{\vec{r}}_2 & \hat{\vec{r}}_3
     *  \end{matrix}\right]
     *  =
     *  \left[\begin{matrix}
     *  \hat{\vec{r}}^2_1               & \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}_1 \hat{\vec{r}}_3 \\
     *  \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}^2_2               & \hat{\vec{r}}_2 \hat{\vec{r}}_3 \\
     *  \hat{\vec{r}}_1 \hat{\vec{r}}_3 & \hat{\vec{r}}_2 \hat{\vec{r}}_3 & \hat{\vec{r}}^3_3
     *  \end{matrix}\right]
     *  \f]
     *  The matrix
     *  \f[
     *  \mathcal{T} = \cos(\theta)\mathbf{I} + (1 - \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + \sin(\theta)\mathbf{R}
     *  \f]
     *  is known as the Rodrigues rotation matrix.
     * @remark
     *  To compute the matrix \f$\mathbf{T}\f$ efficiently, its formula
     *  \f[
     *  \mathcal{T} = \cos(\theta)\mathbf{I} + (1 - \cos(\theta))\left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + \sin(\theta)\mathbf{R}
     *  \f]
     *  is expanded. Let \f$s=\sin(\theta)\f$, \f$c = \cos(\theta)\f$ and \f$t = 1 - \cos(\theta)\f$
     *  \f{align*}{
     *  \mathcal{T}
     * =& c \mathbf{I} + t \left(\hat{\vec{r}} \otimes \hat{\vec{r}}\right) + s \mathbf{R}\\
     * =&
     *  c
     *  \left[\begin{matrix}
     *   1 & 0 & 0 \\
     *   0 & 1 & 0 \\
     *   0 & 0 & 1
     *   \end{matrix}\right]
     *   +
     *   t
     *   \left[\begin{matrix}
     *   \hat{\vec{r}}^2_1               & \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}_1 \hat{\vec{r}}_3 \\
     *   \hat{\vec{r}}_1 \hat{\vec{r}}_2 & \hat{\vec{r}}^2_2               & \hat{\vec{r}}_2 \hat{\vec{r}}_3 \\
     *   \hat{\vec{r}}_1 \hat{\vec{r}}_3 & \hat{\vec{r}}_2 \hat{\vec{r}}_3 & \hat{\vec{r}}^3_3
     *   \end{matrix}\right]
     *   +
     *   s
     *   \left[\begin{matrix}
     *   0                  & -\hat{\vec{r}}_3 &  \hat{\vec{r}}_2 \\
     *   \hat{\vec{r}}_3    & 0                & -\hat{\vec{r}}_1 \\
     *   -\hat{\vec{r}}_2    & \hat{\vec{r}}_1  & 0
     *   \end{matrix}\right]\\
     * =&
     *  \left[\begin{matrix}
     *   c & 0 & 0 \\
     *   0 & c & 0 \\
     *   0 & 0 & c
     *   \end{matrix}\right]
     *   +
     *   \left[\begin{matrix}
     *   t\hat{\vec{r}}^2_1               & t\hat{\vec{r}}_1 \hat{\vec{r}}_2 & t\hat{\vec{r}}_1 \hat{\vec{r}}_3 \\
     *   t\hat{\vec{r}}_1 \hat{\vec{r}}_2 & t\hat{\vec{r}}^2_2               & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 \\
     *   t\hat{\vec{r}}_1 \hat{\vec{r}}_3 & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 & t\hat{\vec{r}}^3_3
     *   \end{matrix}\right]
     *   +
     *   \left[\begin{matrix}
     *   0                 & -s\hat{\vec{r}}_3 &  s\hat{\vec{r}}_2 \\
     *   s\hat{\vec{r}}_3  & 0                 & -s\hat{\vec{r}}_1 \\
     *   -s\hat{\vec{r}}_2 & s\hat{\vec{r}}_1  & 0
     *   \end{matrix}\right]\\
     * =&
     *   \left[\begin{matrix}
     *   t\hat{\vec{r}}^2_1 + c                              & t\hat{\vec{r}}_1 \hat{\vec{r}}_2 - s\hat{\vec{r}}_3 & t\hat{\vec{r}}_1 \hat{\vec{r}}_3 + s\hat{\vec{r}}_2\\
     *   t\hat{\vec{r}}_1 \hat{\vec{r}}_2 + s\hat{\vec{r}}_3 & t\hat{\vec{r}}^2_2 + c                              & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 - s\hat{\vec{r}}_1  \\
     *   t\hat{\vec{r}}_1 \hat{\vec{r}}_3 - s\hat{\vec{r}}_2 & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 + s\hat{\vec{r}}_1 & t\hat{\vec{r}}^3_3 + c
     *   \end{matrix}\right]
     *  \f}
     * @note
     *  In the matrix
     *  \f[
     *  \mathcal{T} =
     *  \left[\begin{matrix}
     *  t\hat{\vec{r}}^2_1 + c                              & t\hat{\vec{r}}_1 \hat{\vec{r}}_2 - s\hat{\vec{r}}_3 & t\hat{\vec{r}}_1 \hat{\vec{r}}_3 + s\hat{\vec{r}}_2\\
     *  t\hat{\vec{r}}_1 \hat{\vec{r}}_2 + s\hat{\vec{r}}_3 & t\hat{\vec{r}}^2_2 + c                              & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 - s\hat{\vec{r}}_1  \\
     *  t\hat{\vec{r}}_1 \hat{\vec{r}}_3 - s\hat{\vec{r}}_2 & t\hat{\vec{r}}_2 \hat{\vec{r}}_3 + s\hat{\vec{r}}_1 & t\hat{\vec{r}}^3_3 + c
     *  \end{matrix}\right]\\
     *  s=\sin(\theta), c = \cos(\theta), t = 1 - \cos(\theta)
     *  \f]
     *  common subexpressions exist which can be eliminated by precomputation:
     *  \f{align*}{
     *  \mathcal{T} =
     *  \left[\begin{matrix}
     *  t_1 \hat{\vec{r}}_1 + c & t_12 - s_3              & t_13 + s_2 \\
     *  t_12 + s_3              & t_2 \hat{\vec{r}}_2 + c & t_23 - s_1 \\
     *  t_13 - s_2              & t_23 + s_1              & t_3 \hat{\vec{r}}_3 + c
     *  \end{matrix}\right]\\
     *  s=\sin(\theta), c = \cos(\theta), t = 1 - \cos(\theta),\\
     *  t_1 = t\hat{\vec{r}}_1, t_2 = t\hat{\vec{r}}_2, t_3 = t\hat{\vec{r}}_3,\\
     *  t_12 = t_1 \hat{\vec{r}}_2, t_13 = t_1 \hat{\vec{r}}_3, t_23 = t_2 \hat{\vec{r}}_3\\
     *  s_1 = s\hat{\vec{r}}_1, s_2 = s\hat{\vec{r}}_2, s_3 = s\hat{\vec{r}}_3
     *  \f}
     *  This implementation performs this form of elimination of common subexpressions.
     */
    static Matrix4f4f rotation(const Vector3f& axis, const Ego::Math::Degrees& angle) {
        return rotation(axis, Ego::Math::Radians(angle));
    }

    static Matrix4f4f rotation(const Vector3f& axis, const Ego::Math::Turns& angle) {
        return rotation(axis, Ego::Math::Radians(angle));
    }

    static Matrix4f4f rotation(const Vector3f& axis, const Ego::Math::Radians& angle) {
        float c = std::cos(angle), s = std::sin(angle);
        float t = 1.0f - c;
        float x = axis[kX], y = axis[kY], z = axis[kZ];
        float xx = x*x, yy = y*y, zz = z*z;
        float l = std::sqrt(xx+yy+zz);
        if (l == 0.0f)
        {
            throw std::invalid_argument("axis vector is zero vector");
        }
        x /= l; y /= l; z /= l;
        float sx = s * x, sy = s * y, sz = s * z,
              tx = t * x, ty = t * y, tz = t * z;
        float txy = tx * y, txz = tx * z, tyz = ty * z;

        return Matrix4f4f
            (
                tx * x + c, txy - sz,   txz + sy,   0,
                txy + sz,   ty * y + c, tyz - sx,   0,
                txz - sy,   tyz + sx,   tz * z + c, 0,
                0,          0,          0,          1
            );
    }

    /**@}*/

    /**
     * @brief
     *  Get a scaling scaling matrix.
     * @param self
     *  this matrix
     * @param s
     *  a scaling vector
     * @return
     *  the matrix
     * @remark
     *  \f[
     *  \left[\begin{matrix}
     *  s_x &  0   & 0   & 0 \\
     *  0   &  s_y & 0   & 0 \\
     *  0   &  0   & s_z & 0 \\
     *  0   &  0   & 0   & 1 \\
     *  \end{matrix}\right]
     *  \f]
     */
    static Matrix4f4f scaling(const Vector3f& s) {
        return
            Matrix4f4f
            (
                s[kX],     0,     0, 0,
                    0, s[kY],     0, 0,
                    0,     0, s[kZ], 0,
                    0,     0,     0, 1
            );
    }
};

} // namespace Math
} // namespace Ego
