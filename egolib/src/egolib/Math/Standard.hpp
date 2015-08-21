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
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 2>> Vector2f;

/// A 3D vector.
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> Vector3f;

/// A 4D vector.
typedef Ego::Math::Vector<Ego::Math::VectorSpace<Ego::Math::Field<float>, 4>> Vector4f;

/// A 3D sphere.
typedef Ego::Math::Sphere<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> Sphere3f;

/// A 3D AABB.
typedef Ego::Math::AABB<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> AABB3f;

/// A 2D AABB.
typedef Ego::Math::AABB<Ego::Math::VectorSpace<Ego::Math::Field<float>, 2>> AABB2f;

/// A 3D cone.
typedef Ego::Math::Cone3<float> Cone3f;

/// A 3D cube.
typedef Ego::Math::Cube<Ego::Math::VectorSpace<Ego::Math::Field<float>, 3>> Cube3f;

/// A 3D plane.
typedef Ego::Math::Plane3<float> Plane3f;

#ifdef _DEBUG
namespace Ego {
namespace Debug {

template <>
void validate<float>(const char *file, int line, const float& object);

template <>
void validate<::Vector2f>(const char *file, int line, const ::Vector2f& object);

template <>
void validate<::Vector3f>(const char *file, int line, const ::Vector3f& object);

template <>
void validate<::Vector4f>(const char *file, int line, const ::Vector4f& object);

template <>
void validate<::AABB3f>(const char *file, int line, const ::AABB3f& object);

template <>
void validate<::Sphere3f>(const char *file, int line, const ::Sphere3f& object);

template <>
void validate<::Cube3f>(const char *file, int line, const ::Cube3f& object);

} // namespace Debug
} // namespace Ego
#endif

/// @todo Remove this.
float fvec3_decompose(const Vector3f& src, const Vector3f& vnrm, Vector3f& vpara, Vector3f& vperp);



