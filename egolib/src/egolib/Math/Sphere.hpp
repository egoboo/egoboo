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

/// @file  egolib/Math/Sphere.h
/// @brief Spheres.

#pragma once

#include "egolib/Math/Vector.hpp"
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
struct Sphere : public Translatable<typename _EuclideanSpaceType::VectorSpaceType> {
public:
    /// @brief The Euclidean space over which the lines are defined.
    typedef _EuclideanSpaceType EuclideanSpaceType;
    /// The vector space type (of the Euclidean space).
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    /// The scalar field type (of the vector space).
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    /// The vector type (of the vector space).
    typedef typename EuclideanSpaceType::VectorType VectorType;
    /// The scalar type (of the scalar field).
    typedef typename EuclideanSpaceType::ScalarType ScalarType;
    /// @brief @a MyType is the type of this template/template specialization.
    typedef Sphere<_EuclideanSpaceType> MyType;

private:

    /**
     * @brief
     *  The center of the sphere.
     */
    VectorType _center;

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
        : _center(VectorType::zero()), _radius(0) {
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
    Sphere(const VectorType& center, const ScalarType& radius)
        : _center(center), _radius(radius) {
        if (_radius < 0) {
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
    const VectorType& getCenter() const {
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
    void setCenter(const VectorType& center) {
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
    const ScalarType& getRadiusSquared() const {
        return _radius * _radius;
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
    
#if 0
    /**
     * @brief
     *  Get if this sphere intersects with a point.
     * @param other
     *  the point
     * @return
     *  @a true if this sphere intersects with the point,
     *  @a false otherwise
     * @remark
     *  A sphere \f$(c,r)\f$ with the center $c$ and the radius $r$
     *  and a point \f$p\f$ intersect if \f$|p - c| \leq r\f$ holds.
     *  That condition is equivalent to the condition \f$|p - c|^2
     *  \leq r^2\f$ but the latter is more efficient to test (two
     *  multiplications vs. one square root).
     */
    bool intersects(const VectorType& other) const {
        // Get the squared distance other the point and the center of the sphere.
        float distance_2 = (_center - other).length_2();
        // Get the squared radius of the sphere.
        float radius_2 = _radius * _radius;
        // If the squared distance beween the point and the center of the sphere
        // is smaller than or equal to the squared radius of the sphere ...
        if (distance_2 <= radius_2) {
            // ... the sphere and the point intersect.
            return true;
        }
        // Otherwise they don't intersect.
        return false;
    }
#endif

#if 0
    /**
     * @brief
     *  Get if this sphere intersects with another sphere.
     * @param other
     *  the other sphere
     * @return
     *  @a true if this sphere intersects with the other sphere,
     *  @a false otherwise
     * @remark
     *  Two spheres \f$(c_0,r_0)\f$ and \f$(c_1,r_1)\f$ with the
     *  centers \f$c_0\f$ and \f$c_1\f$ and the radii \f$r_0\f$
     *  and \f$r_1\f$ intersect if \f$|c_1 - c_0| \leq r_0 + r_1\f$
     *  holds. That condition is equivalent to the condition
     *  \f$|c_1 - c_0|^2 \leq (r_0 + r_1)^2\f$ but the latter
     *  is more efficient to test (two multiplications vs. one
     *  square root).
     */
    bool intersects(const MyType& other) const {
        // Get the squared distance between the centers of the two spheres.
        float distance_2 = (_center - other._center).length_2();
        // Get the squared sum of the radiis of the two spheres.
        float sumOfRadii = _radius + other._radius;
        float sumOfRadii_2 = sumOfRadii * sumOfRadii;
        // If the squared distance beween the centers of the spheres
        // is smaller than or equal to the squared sum of the radii of the spheres ...
        if (distance_2 <= sumOfRadii_2) {
            // ... the spheres intersect.
            return true;
        }
        // Otherwise they don't intersect.
        return false;
    }
#endif

	/** @copydoc Ego::Math::translatable */
	void translate(const VectorType& t) override {
		_center += t;
	}

};

} // namespace Math
} // namespace Ego
