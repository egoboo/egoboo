#include "egolib/Math/Standard.hpp"
#include "egolib/Log/_Include.hpp"
#include "egolib/_math.h"
#include "egolib/Float.hpp"

#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
void validate<float>(const char *file, int line, const float& object) {
    if (float_bad(object)) {
		std::ostringstream os;
		os << file << ":" << line << ": invalid floating point value" << std::endl;
		Log::get().error("%s", os.str().c_str());
		throw std::runtime_error(os.str());
    }
}

template <>
void validate<::Vector2f>(const char *file, int line, const ::Vector2f& object) {
    for (size_t i = 0; i < 2; ++i) {
        validate(file, line, object[i]);
    }
}

template <>
void validate<::Vector3f>(const char *file, int line, const ::Vector3f& object) {
    for (size_t i = 0; i < 3; ++i) {
        validate(file, line, object[i]);
    }
}

template <>
void validate<::Vector4f>(const char *file, int line, const ::Vector4f& object) {
    for (size_t i = 0; i < 4; ++i) {
        validate(file, line, object[i]);
    }
}

template <>
void validate<::AABB3f>(const char *file, int line, const ::AABB3f& object) {
    validate(file, line, object.getMin());
    validate(file, line, object.getMax());
}

template <>
void validate<::Sphere3f>(const char *file, int line, const ::Sphere3f& object) {
    validate(file, line, object.getCenter());
    validate(file, line, object.getRadius());
}

template <>
void validate<::Cube3f>(const char *file, int line, const ::Cube3f& object) {
    validate(file, line, object.getCenter());
    validate(file, line, object.getSize());
}

} // namespace Debug
} // namespace Ego
#endif

void mat_ScaleXYZ_RotateXYZ_TranslateXYZ_SpaceFixed(Matrix4f4f& DST, const Vector3f& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const Vector3f& translate)
{
    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

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
}

void mat_ScaleXYZ_RotateXYZ_TranslateXYZ_BodyFixed(Matrix4f4f& DST, const Vector3f& scale, const TURN_T turn_z, const TURN_T turn_x, const TURN_T turn_y, const Vector3f& translate)
{
    /// @details Transpose the SpaceFixed representation and invert the angles to get the BodyFixed representation

    float cx = turntocos[turn_x & TRIG_TABLE_MASK];
    float sx = turntosin[turn_x & TRIG_TABLE_MASK];
    float cy = turntocos[turn_y & TRIG_TABLE_MASK];
    float sy = turntosin[turn_y & TRIG_TABLE_MASK];
    float cz = turntocos[turn_z & TRIG_TABLE_MASK];
    float sz = turntosin[turn_z & TRIG_TABLE_MASK];

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
}

void mat_FourPoints(Matrix4f4f& dst, const Vector3f& ori, const Vector3f& wid, const Vector3f& frw, const Vector3f& up, const float scale)
{
	Vector3f vWid = wid - ori;
	Vector3f vUp = up - ori;
	Vector3f vFor = frw - ori;

    vWid.normalize();
    vUp.normalize();
    vFor.normalize();

	dst(0, 0) = -scale * vWid[kX];  // HUK
	dst(1, 0) = -scale * vWid[kY];  // HUK
	dst(2, 0) = -scale * vWid[kZ];  // HUK
	dst(3, 0) = 0.0f;

	dst(0, 1) = scale * vFor[kX];
	dst(1, 1) = scale * vFor[kY];
	dst(2, 1) = scale * vFor[kZ];
	dst(3, 1) = 0.0f;

	dst(0, 2) = scale * vUp[kX];
	dst(1, 2) = scale * vUp[kY];
	dst(2, 2) = scale * vUp[kZ];
	dst(3, 2) = 0.0f;

	dst(0, 3) = ori[kX];
	dst(1, 3) = ori[kY];
	dst(2, 3) = ori[kZ];
	dst(3, 3) = 1.0f;
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
