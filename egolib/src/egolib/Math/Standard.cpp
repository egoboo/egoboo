#include "egolib/Math/Standard.hpp"
#include "egolib/log.h"
#include "egolib/_math.h"


#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
void validate<float>(const char *file, int line, const float& object) {
    if (float_bad(object)) {
        log_error("%s:%d: invalid floating point value\n", file, line);
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
