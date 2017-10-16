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

/// @file egolib/Math/Ray.hpp
/// @brief Rays.
/// @author Michael Heilmann

#pragma once

#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief A ray has an origin point \f$O\f$ and an unit direction vector \$\vec{d}\f$.
 * The set of points of a ray is given by \f$\left\{ O + t \vec{d} | t \in \subseteq \mathbb{R}_{\geq 0} \right\}$.
 */
template <typename _EuclideanSpaceType>
struct Ray : public id::equal_to_expr<Ray<_EuclideanSpaceType>> {
public:
    Ego_Math_EuclideanSpace_CommonDefinitions(Ray);

private:
    /**
     * @brief The origin of this ray.
     */
    PointType origin;
    /**
     * @brief The direction of this ray.
     */
    VectorType direction;

public:
    /**
     * @brief Construct this ray with the specified origin point \f$O\f$ and axis vector \f$\vec{a}\f$.
     * @param origin the origin point \f$O\f$
     * @param direction the direction vector \f$\vec{d}\f$
     * @throw id::runtime_error if the direction vector \f$\vec{d}\f$ is the zero vector 
     */
    Ray(const PointType& origin, const VectorType& direction)
        : origin(origin), direction(direction) {
        auto length = id::euclidean_norm(this->direction);
        if (length == id::zero<ScalarType>()) {
            throw id::runtime_error(__FILE__, __LINE__, "direction vector is zero vector");
        }
        this->direction *= id::one<ScalarType>() / length;
    }

    /** 
     * @brief Construct this ray with the values of another ray.
     * @param other the other ray
     */
    Ray(const Ray& other)
        : origin(other.origin), direction(other.direction) {
        /* Intentionally empty. */
    }

    /**
     * @brief Assign this ray with the values of another ray.
     * @param other the other ray
     * @return this ray
     */
    Ray operator=(const Ray& other) {
        origin = other.origin;
        direction = other.direction;
        return *this;
    }

public:
    /** 
     * @brief Get the origin of this ray.
     * @return the origin of this ray
     */
    const PointType& getOrigin() const {
        return origin;
    }

    /**
     * @brief Get the unit direction vector of this ray.
     * @return the unit direction vector of this ray
     */
    const VectorType& getDirection() const {
        return direction;
    }

public:
    // CRTP
    bool equal_to(const MyType& other) const
    {
        return origin == other.origin
            && direction == other.direction;
    }

}; // struct Ray

} // namespace Math
} // namespace Ego

namespace id {

/// @brief Specialization of id::enclose_functor.
/// Encloses a ray in a ray.
/// @detail The ray \f$b\f$ enclosing a ray \f$a\f$ is \f$a\f$ itself i.e. \f$b = a\f$.
/// @tparam E the Euclidean space type of the geometries
template <typename E>
struct enclose_functor<Ego::Math::Ray<E>,
	                   Ego::Math::Ray<E>>
{
	auto operator()(const Ego::Math::Ray<E>& source) const
	{ return source; }
}; // struct enclose_functor

/// @brief Specialization of id::translate_functor.
/// Translates a ray.
/// @tparam E the Euclidean space type of the geometry
template <typename E>
struct translate_functor<Ego::Math::Ray<E>,
                         typename E::VectorType>
{
	auto operator()(const Ego::Math::Ray<E>& x,
		            const typename E::VectorType& t) const
	{
		return Ego::Math::Ray<E>(x.getOrigin() + t, x.getDirection());
	}
}; // struct translate_functor

} // namespace id
