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
#include "egolib/Math/Translatable.hpp"

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
        : _center(PointType::zero()), _radius(0) {
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
    bool operator==(const MyType& other) const {
        return _center == other._center
            && _radius == other._radius;
    }

    bool operator!=(const MyType& other) const {
        return _center != other._center
            || _radius != other._radius;
    }

	/** @copydoc Ego::Math::translatable */
	void translate(const VectorType& t) override {
		_center += t;
	}

}; // struct Sphere

template <typename _EuclideanSpaceType>
struct Translate<Sphere<_EuclideanSpaceType>> {
    typedef Sphere<_EuclideanSpaceType> X;
    typedef typename _EuclideanSpaceType::VectorSpaceType T;
    X operator()(const X& x, const T& t) const {
        return X(x.getCenter() + t, x.getRadius());
    }
};

template <typename _EuclideanSpaceType>
Sphere<_EuclideanSpaceType> translate(const Sphere<_EuclideanSpaceType>& x, const typename _EuclideanSpaceType::VectorType& t) {
    Translate<Sphere<_EuclideanSpaceType>> f;
    return f(x, t);
}

} // namespace Math
} // namespace Ego
