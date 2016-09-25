#include "egolib/Math/Standard.hpp"
#include "egolib/Log/_Include.hpp"
#include "egolib/_math.h"
#include "egolib/Float.hpp"

Matrix4f4f mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(const Vector3f& scale, const Facing& turn_z, const Facing& turn_x, const Facing& turn_y, const Vector3f& translate)
{
    float cx = std::cos(turn_x);
    float sx = std::sin(turn_x);
    float cy = std::cos(turn_y);
    float sy = std::sin(turn_y);
    float cz = std::cos(turn_z);
    float sz = std::sin(turn_z);

    Matrix4f4f DST;
    DST(0,0) = scale[kX] * (cz * cy);
    DST(1,0) = scale[kX] * (cz * sy * sx + sz * cx);
    DST(2,0) = scale[kX] * (sz * sx - cz * sy * cx);
    DST(3,0) = 0.0f;

    DST(0,1) = scale[kY] * (-sz * cy);
    DST(1,1) = scale[kY] * (-sz * sy * sx + cz * cx);
    DST(2,1) = scale[kY] * (sz * sy * cx + cz * sx);
    DST(3,1) = 0.0f;

    DST(0,2) = scale[kZ] * (sy);
    DST(1,2) = scale[kZ] * (-cy * sx);
    DST(2,2) = scale[kZ] * (cy * cx);
    DST(3,2) = 0.0f;

    DST(0,3) = translate[kX];
    DST(1,3) = translate[kY];
    DST(2,3) = translate[kZ];
    DST(3,3) = 1.0f;

    return DST;
}

Matrix4f4f mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(const Vector3f& scale, const Facing& turn_z, const Facing& turn_x, const Facing& turn_y, const Vector3f& translate)
{
    float cx = std::cos(turn_x);
    float sx = std::sin(turn_x);
    float cy = std::cos(turn_y);
    float sy = std::sin(turn_y);
    float cz = std::cos(turn_z);
    float sz = std::sin(turn_z);

    Matrix4f4f DST;
    DST(0,0) = scale[kX] * (cz * cy - sz * sy * sx);
    DST(1,0) = scale[kX] * (sz * cy + cz * sy * sx);
    DST(2,0) = scale[kX] * (-cx * sy);
    DST(3,0) = 0.0f;

    DST(0,1) = scale[kY] * (-sz * cx);
    DST(1,1) = scale[kY] * (cz * cx);
    DST(2,1) = scale[kY] * (sx);
    DST(3,1) = 0.0f;

    DST(0,2) = scale[kZ] * (cz * sy + sz * sx * cy);
    DST(1,2) = scale[kZ] * (sz * sy - cz * sx * cy);
    DST(2,2) = scale[kZ] * (cy * cx);
    DST(3,2) = 0.0f;

    DST(0,3) = translate[kX];
    DST(1,3) = translate[kY];
    DST(2,3) = translate[kZ];
    DST(3,3) = 1.0f;

    return DST;
}

Vector3f mat_getTranslate(const Matrix4f4f& mat)
{
    return Vector3f(mat(0, 3), mat(1, 3), mat(2, 3));
}

Vector3f mat_getChrUp(const Matrix4f4f& mat)
{
    return Vector3f(mat(0, 2), mat(1, 2), mat(2, 2));
}

Vector3f mat_getChrForward(const Matrix4f4f& mat)
{
    return Vector3f(-mat(0, 0), -mat(1, 0), -mat(2, 0));
}

Vector3f mat_getChrRight(const Matrix4f4f& mat)
{
    return Vector3f(mat(0, 1), mat(1, 1), mat(2, 1));
}

bool mat_getCamUp(const Matrix4f4f& mat, Vector3f& up)
{
    up[kX] = mat(1,0);
    up[kY] = mat(1,1);
    up[kZ] = mat(1,2);

    return true;
}

bool mat_getCamRight(const Matrix4f4f& mat, Vector3f& right)
{
    right[kX] = -mat(0,0);
    right[kY] = -mat(0,1);
    right[kZ] = -mat(0,2);

    return true;
}

bool mat_getCamForward(const Matrix4f4f& mat, Vector3f& forward)
{
    forward[kX] = -mat(2,0);
    forward[kY] = -mat(2,1);
    forward[kZ] = -mat(2,2);

    return true;
}

void dump_matrix(const Matrix4f4f& a)
{
    for (size_t i = 0; i < 4; ++i)
    {
        printf("  ");

		for (size_t j = 0; j < 4; ++j)
        {
            printf("%f ", a(i,j));
        }
        printf("\n");
    }
}

float fvec3_decompose(const Vector3f& A, const Vector3f& vnrm, Vector3f& vpara, Vector3f& vperp)
{
    /// @author BB
    /// @details the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

    // if this is true, there is no reason to run this function
    float dot = A.dot(vnrm);

    if (0.0f == dot)
    {
		vpara = Vector3f::zero();
		vperp = A;
    }
    else
    {
		vpara = vnrm * dot;
		vperp = A - vpara;
    }

    return dot;
}
