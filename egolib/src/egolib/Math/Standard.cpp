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
void validate<::fvec2_t>(const char *file, int line, const ::fvec2_t& object) {
    for (size_t i = 0; i < 2; ++i) {
        validate(file, line, object[i]);
    }
}

template <>
void validate<::fvec3_t>(const char *file, int line, const ::fvec3_t& object) {
    for (size_t i = 0; i < 3; ++i) {
        validate(file, line, object[i]);
    }
}

template <>
void validate<::fvec4_t>(const char *file, int line, const ::fvec4_t& object) {
    for (size_t i = 0; i < 4; ++i) {
        validate(file, line, object[i]);
    }
}

template <>
void validate<::aabb_t>(const char *file, int line, const ::aabb_t& object) {
    validate(file, line, object.getMin());
    validate(file, line, object.getMax());
}

template <>
void validate<::sphere_t>(const char *file, int line, const ::sphere_t& object) {
    validate(file, line, object.getCenter());
    validate(file, line, object.getRadius());
}

template <>
void validate<::cube_t>(const char *file, int line, const ::cube_t& object) {
    validate(file, line, object.getCenter());
    validate(file, line, object.getSize());
}

} // namespace Debug
} // namespace Ego
#endif

float fvec3_decompose(const fvec3_t& A, const fvec3_t& vnrm, fvec3_t& vpara, fvec3_t& vperp)
{
    /// @author BB
    /// @details the normal (vnrm) is assumed to be normalized. Try to get this as optimized as possible.

    float dot;

    // if this is true, there is no reason to run this function
    dot = A.dot(vnrm);

    if (0.0f == dot)
    {
        {
            vpara[kX] = 0.0f;
            vpara[kY] = 0.0f;
            vpara[kZ] = 0.0f;

            vperp[kX] = A[kX];
            vperp[kY] = A[kY];
            vperp[kZ] = A[kZ];
        }
    }
    else
    {
        {
            vpara[kX] = dot * vnrm[kX];
            vpara[kY] = dot * vnrm[kY];
            vpara[kZ] = dot * vnrm[kZ];

            vperp[kX] = A[kX] - vpara[kX];
            vperp[kY] = A[kY] - vpara[kY];
            vperp[kZ] = A[kZ] - vpara[kZ];

        }
    }

    return dot;
}
