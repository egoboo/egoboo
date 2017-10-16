//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file egolib/Math/Sphere.hpp
/// @brief Spheres.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief
 *  A sphere.
 * @remark
 *  The terms the/a "sphere object" and the/a "sphere" are synonyms.
 */
template <typename _EuclideanSpaceType>
struct Sphere : public id::equal_to_expr<Sphere<_EuclideanSpaceType>> {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Sphere);

private:

    /**
     * @brief
     *  The center of the sphere.
     */
    PointType _center;

    /**
     * @brief
     *  The radius of the sphere.
     * @invariant
     *  Greater than or equal to @a 0.
     */
    ScalarType _radius;

public:

    /**
     * @brief
     *  Construct this sphere assigning it the default values of a sphere.
     * @post
     *  This sphere was constructed with the default values of a sphere.
     * @remark
     *  The default values of a sphere are the center of @a (0,0,0) and the radius of @a 0.
     */
    Sphere()
        : _center(id::zero<PointType>()), _radius(0) {
        /* Intentionally empty. */
    }

    /**
    * @brief
    *  Construct this sphere with specified values.
    * @param center
    *  the center of the sphere
    * @param radius
    *  the radius of the sphere
    * @throw std::domain_error
    *  if the radius is negative
    * @pre
    *  The radius is not negative.
    * @post
    *  The sphere was constructed with the specified values.
    */
    Sphere(const PointType& center, const ScalarType& radius)
        : _center(center), _radius(radius) {
        if (_radius < id::zero<ScalarType>()) {
            throw std::domain_error("sphere radius is negative");
        }
    }

    /**
     * @brief
     *  Construct this sphere with the values of another sphere.
     * @param other
     *  the other sphere
     * @post
     *  This sphere was constructed with the values of the other sphere.
     */
    Sphere(const MyType& other)
        : _center(other._center), _radius(other._radius) {
        /* Intentionally empty. */
    }

public:

    /**
     * @brief
     *  Get the center of this sphere.
     * @return
     *  the center of this sphere
     */
    const PointType& getCenter() const {
        return _center;
    }

    /**
     * @brief
     *  Set the center of this sphere.
     * @param center
     *  the center
     * @post
     *  The sphere was assigned with the center.
     */
    void setCenter(const PointType& center) {
        _center = center;
    }

    /**
     * @brief
     *  Get the radius of this sphere.
     * @return
     *  the radius of this sphere
     */
    const ScalarType& getRadius() const {
        return _radius;
    }

    /**
     * @brief
     *  Get the squared radius of this sphere.
     * @return
     *  the squared radius of this sphere
     */
    ScalarType getRadiusSquared() const {
        return _radius * _radius;
    }

    /**
     * @brief
     *  Get the diameter of this sphere.
     * @return
     *  the diameter of this sphere
     */
    ScalarType getDiameter() const {
        return getRadius() * 2.0;
    }

    /**
     * @brief
     *  Get the squared diameter of this sphere.
     * @return
     *  the squared diameter of this sphere
     */
    ScalarType getDiameterSquared() const {
        auto diameter = getDiameter();
        return diameter * diameter;
    }

    /**
     * @brief
     *  Set the radius of this sphere.
     * @param radius
     *  the radius
     * @pre
     *  The radius must be non-negative.
     * @throw std::domain_error
     *  if the radius is negative
     * @post
     *  If an exception is raised, the sphere's radius was not modified.
     *  Otherwise, the sphere was assigned with the radius.
     */
    void setRadius(const ScalarType& radius) {
        if (radius < 0) {
            throw std::domain_error("sphere radius is negative");
        }
        _radius = radius;
    }

    /**
     * @brief
     *  Assign this sphere with the values of another sphere.
     * @param other
     *  the other sphere
     * @post
     *  This sphere was assigned with the values of the other sphere.
     */
    void assign(const MyType& other) {
        _radius = other._radius;
        _center = other._center;
    }

public:
	// CRTP
    bool equal_to(const MyType& other) const {
        return _center == other._center
            && _radius == other._radius;
    }

}; // struct Sphere



} // namespace Math
} // namespace Ego

namespace id {

/// @brief Specialization of id::enclose_functor enclosing a sphere into a sphere.
/// @detail The sphere \f$b\f$ enclosing a sphere \f$a\f$ is \f$a\f$ itself i.e. \f$b = a\f$.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::Sphere<E>,
	                   Ego::Math::Sphere<E>>
{
	auto operator()(const Ego::Math::Sphere<E>& source) const
	{ return source; }
}; // struct enclose_functor

/// @brief Specialization of id::is_enclosing_functor.
/// Determines wether a sphere contains a point.
/// @remark A sphere \f$(c,r)\f$ with the center $c$ and the radius $r$
/// contains a point \f$p\f$ if \f$|p - c| \leq r\f$ holds.
/// That condition is equivalent to the condition \f$|p - c|^2
/// \leq r^2\f$ but the latter is more efficient to test (two
/// multiplications vs. one square root).
template <typename E>
struct is_enclosing_functor<Ego::Math::Sphere<E>,
	                        typename E::PointType>
{
	bool operator()(const Ego::Math::Sphere<E>& a,
		            const typename E::PointType& b) const
	{
		// Get the squared distance from the point to the center of the sphere.
		auto distanceSquared = id::squared_euclidean_norm(a.getCenter() - b);
		// Get the squared radius of the sphere.
		auto radiusSquared = a.getRadiusSquared();
		// If the squared distance beween the point and the center of the sphere
		// is smaller than or equal to the squared radius of the sphere ...
		if (distanceSquared <= radiusSquared)
		{
			// ... the sphere contains the point.
			return true;
		}
		// Otherwise the sphere does not contain the point.
		return false;
	}
}; // struct is_enclosing_functor

/// @brief Specialization of id::is_enclosing_functor.
/// Determine if a sphere contains another sphere.
/// @remark
/// A sphere \f$S\f$ with center \f$c\f$ and radius \f$r\f$ contains
/// another sphere \f$S'\$ with center \f$c'\f$ and radius \f$r'\f$
/// if \f$|c-c'|+r' \leq r\f$ holds.
template <typename E>
struct is_enclosing_functor<Ego::Math::Sphere<E>,
                            Ego::Math::Sphere<E>>
{
	bool operator()(const Ego::Math::Sphere<E>& a,
		            const Ego::Math::Sphere<E>& b) const
	{
		return id::euclidean_norm(a.getCenter() - b.getCenter()) + b.getRadius()
			<= a.getRadius();
	}
}; // struct is_enclosing_functor

/// @brief Specialization of id::translate_functor.
/// Translates an sphere.
/// @tparam E the Euclidean space type of the geometry
template <typename E>
struct translate_functor<Ego::Math::Sphere<E>,
	                     typename E::VectorType>
{
	auto operator()(const Ego::Math::Sphere<E>& x,
		            const typename E::VectorType& t) const
	{ return Ego::Math::Sphere<E>(x.getCenter() + t, x.getRadius()); }
}; // struct translate_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if two spheres intersect.
/// @remark Two spheres \f$X\f$ and \f$Yf$ with the
/// centers \f$X_C\f$ and \f$Y_Y\f$ and the radii \f$X_r\f$
/// and \f$Y_r\f$ intersect if \f$|X_c - Y_c| \leq X_r + Y_r\f$
/// holds. That condition is equivalent to the condition
/// \f$|X_C - Y_C|^2 \leq (X_ + Y_r)^2\f$. In terms of
/// an implementation the former is usually less efficient
/// than the latter.
template <typename E>
struct is_intersecting_functor<Ego::Math::Sphere<E>,
	                           Ego::Math::Sphere<E>>
{
	bool operator()(const Ego::Math::Sphere<E>& a,
		            const Ego::Math::Sphere<E>& b) const
	{
		// Get the squared distance between the centers of the two spheres.
		auto distanceSquared = id::squared_euclidean_norm(a.getCenter() - b.getCenter());
		// Get the squared sum of the radiis of the two spheres.
		auto sumOfRadii = a.getRadius() + b.getRadius();
		auto sumOfRadiiSquared = sumOfRadii * sumOfRadii;
		// If the squared distance beween the centers of the spheres
		// is smaller than or equal to the squared sum of the radii of the spheres ...
		if (distanceSquared <= sumOfRadiiSquared)
		{
			// ... the spheres intersect.
			return true;
		}
		// Otherwise they don't intersect.
		return false;
	}
}; // struct is_intersecting_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if a sphere and a point intersect.
/// @remark A sphere \f$A\f$ with the center $C$ and the radius $r$
/// and a point \f$P\f$ intersect if \f$|P - C| \leq r\f$ holds.
/// That condition is equivalent to the condition \f$|p - c|^2
/// \leq r^2\f$ but the latter is more efficient to test (two
/// multiplications vs. one square root).
template <typename E>
struct is_intersecting_functor<Ego::Math::Sphere<E>,
	                           typename E::PointType>
{
	bool operator()(const Ego::Math::Sphere<E>& a,
		            const typename E::PointType& b) const
	{
		// Get the squared distance from the point to the center of the sphere.
		float distanceSquared = id::squared_euclidean_norm(a.getCenter() - b);
		// Get the squared radius of the sphere.
		float radiusSquared = a.getRadiusSquared();
		// If the squared distance beween the point and the center of the sphere
		// is smaller than or equal to the squared radius of the sphere ...
		if (distanceSquared <= radiusSquared)
		{
			// ... the sphere and the point intersect.
			return true;
		}
		// Otherwise the sphere and the point do not intersect.
		return false;
	}
}; // struct is_intersecting_functor

/// @brief Specialization of id::is_intersecting_functor.
/// Determines if a point and a sphere intersect.
/// @remark The method for determinating if a sphere and a point intersect is
/// commutative. By swapping the arguments that method can be reused to determine if a
/// point and a sphere intersect.
template <typename E>
struct is_intersecting_functor<typename E::PointType,
	                           Ego::Math::Sphere<E>>
{
	bool operator()(const typename E::PointType& a, const Ego::Math::Sphere<E>& b) const
	{ return is_intersecting(b, a); }
}; // struct is_intersecting_functor

} // namespace id

