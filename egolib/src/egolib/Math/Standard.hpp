#pragma once

#include "egolib/Math/AABB.hpp"
#include "egolib/Math/Cone.hpp"
#include "egolib/Math/Cube.hpp"
#include "egolib/Math/Plane.hpp"
#include "egolib/Math/Sphere.h"
#include "egolib/Math/Vector.hpp"

/**
 * @brief
 *  Enumerated indices for the elements of vectors.
 */
enum {
    kX = 0, kY, kZ, kW
};


/// A 2D vector.
typedef Ego::Math::Vector<float, 2> fvec2_t;

/// A 3D vector.
typedef Ego::Math::Vector<float, 3> fvec3_t;

/// A 4D vector.
typedef Ego::Math::Vector<float, 4> fvec4_t;

/// A 3D sphere.
typedef Ego::Math::Sphere<float, 3> sphere_t;

/// A 3D AABB.
typedef Ego::Math::AABB<float, 3> aabb_t;

/// A 3D cone.
typedef Ego::Math::Cone3<float> cone_t;

/// A 3D cube.
typedef Ego::Math::Cube<float, 3> cube_t;

/// A 3D plane.
typedef Ego::Math::Plane3<float> plane_t;

#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
void validate<float>(const char *file, int line, const float& object);

template <>
void validate<::fvec2_t>(const char *file, int line, const ::fvec2_t& object);

template <>
void validate<::fvec3_t>(const char *file, int line, const ::fvec3_t& object);

template <>
void validate<::fvec4_t>(const char *file, int line, const ::fvec4_t& object);

template <>
void validate<::aabb_t>(const char *file, int line, const ::aabb_t& object);

template <>
void validate<::sphere_t>(const char *file, int line, const ::sphere_t& object);

template <>
void validate<::cube_t>(const char *file, int line, const ::cube_t& object);

} // namespace Debug
} // namespace Ego
#endif

/// @todo Remove this.
float fvec3_decompose(const fvec3_t& src, const fvec3_t& vnrm, fvec3_t& vpara, fvec3_t& vperp);



