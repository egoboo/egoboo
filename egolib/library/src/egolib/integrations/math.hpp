#pragma once

#include "idlib/math_geometry.hpp"

namespace Ego {

/// @brief Project a vector (x,y,z) to a vector (x,y).
/// @tparam S the scalar typedef
/// @param v the vector (x,y,z)
/// @return the vector (x,y)
template <typename S>
auto xy(const idlib::vector<S, 3>& v)
{ return idlib::vector<S, 2>(v.x(), v.y()); }

template <typename V, typename W>
auto dot(V&& v, W&& w)
{ return idlib::dot_product(std::forward<V>(v), std::forward<W>(w)); }

template <typename V, typename W>
auto cross(V&& v, W&& w)
{ return idlib::cross_product(std::forward<V>(v), std::forward<W>(w)); }

template <typename V>
auto normalize(V&& v)
{ return idlib::normalize(std::forward<V>(v), idlib::euclidean_norm_functor<std::decay_t<V>>()); }

/// single-precision floating-point 2d vector.
using Vector2f = idlib::vector<float, 2>;
/// single-precision floating-point 3d vector.
using Vector3f = idlib::vector<float, 3>;
/// single-precision floating-point 4d vector.
using Vector4f = idlib::vector<float, 4>;

/// single-precision floating-point 2d point.
using Point2f = idlib::point<Vector2f>;
/// single-precision floating-point 3d point.
using Point3f = idlib::point<Vector3f>;
/// single-precision floating-point 4d point.
using Point4f = idlib::point<Vector4f>;

/// A \f$3\f$-order floating point matrix.
using Matrix3f3f = idlib::matrix<float, 3, 3>;
/// A \f$4\f$-order floating point matrix.
using Matrix4f4f = idlib::matrix<float, 4, 4>;

/// single-precision floating-point 2d axis aligned box.
using AxisAlignedBox2f = idlib::axis_aligned_box<Point2f>;
/// single-precision floating-point rectangle (i.e. an axis aligned box in 2d),
using Rectangle2f = AxisAlignedBox2f;
/// A 3D axis aligned box.
using AxisAlignedBox3f = idlib::axis_aligned_box<Point3f>;

/// single-precision floating-point 2d sphere.
using Sphere2f = idlib::sphere<Point2f>;
/// single-precision floating-point circle (i.e. a sphere in 2d).
using Circle2f = Sphere2f;
/// single-precision floating-point 3d sphere.
using Sphere3f = idlib::sphere<Point3f>;

/// single-precision floating-point 2d line.
using Line2f = idlib::line<Point2f>;
/// single-precision floating-point 3d line.
using Line3f = idlib::line<Point3f>;

/// single-precision floating-point 2d ray.
using Ray2f = idlib::ray<Point2f>;
/// single-precision floating-point 3d ray.
using Ray3f = idlib::ray<Point3f>;

/// single-precision floating-point 3d plane.
using Plane3f = idlib::plane<Point3f>;

/// A 3D cone.
using Cone3f = idlib::cone<Point3f>;

/// A 3D axis aligned cube.
using AxisAlignedCube3f = idlib::axis_aligned_cube<Point3f>;

/// The type of an angle in degrees.
/// See idlib::degrees for more information.
using Degrees = idlib::angle<float, idlib::degrees>;

/// The type of an angle in radians.
/// See idlib::radians for more information.
using Radians = idlib::angle<float, idlib::radians>;

/// The type of an angle in turns.
/// See idlib::turns for more information.
using Turns = idlib::angle<float, idlib::turns>;

/// A \f$3\f$-order floating point matrix.
using Matrix3f3f = idlib::matrix<float, 3, 3>;
/// A \f$4\f$-order floating point matrix.
using Matrix4f4f = idlib::matrix<float, 4, 4>;

} // namespace Ego
