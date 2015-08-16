#pragma once

#include "egolib/Math/AABB.hpp"
#include "egolib/Math/Cone3.hpp"
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
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 2>> fvec2_t;
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 2>> Vector2f;

/// A 3D vector.
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> fvec3_t;
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> Vector3f;

/// A 4D vector.
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 4>> fvec4_t;

/// A 3D sphere.
typedef Ego::Math::Sphere<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> sphere_t;

/// A 3D AABB.
typedef Ego::Math::AABB<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> aabb_t;
typedef Ego::Math::AABB<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> AABB_3D;

/// A 2D AABB.
typedef Ego::Math::AABB<Ego::Math::VectorSpace<Ego::Math::Field<float>, 2>> AABB_2D;

/// A 3D cone.
typedef Ego::Math::Cone3<float> cone_t;

/// A 3D cube.
typedef Ego::Math::Cube<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> cube_t;

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



