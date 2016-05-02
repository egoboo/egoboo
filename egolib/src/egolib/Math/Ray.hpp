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

/// @file  egolib/Math/Ray.hpp
/// @brief Rays.

#pragma once

#include "egolib/Math/EuclideanSpace.hpp"

namespace Ego {
namespace Math {

/**
 * @brief A ray has an origin point \f$O\f$ and an unit direction vector \$\vec{d}\f$.
 * The set of points of a ray is given by \f$\left\{ O + t \vec{d} | t \in \subseteq \mathbb{R}_{\geq 0} \right\}$.
 */
template <typename _EuclideanSpaceType>
struct Ray : public Translatable<typename _EuclideanSpaceType::VectorSpaceType> {
public:
    /// @brief The Euclidean space over which the cones are defined.
    typedef _EuclideanSpaceType EuclideanSpaceType;
    /// The vector space type (of the Euclidean space).
    typedef typename EuclideanSpaceType::VectorSpaceType VectorSpaceType;
    /// The scalar field type (of the vector space).
    typedef typename EuclideanSpaceType::ScalarFieldType ScalarFieldType;
    /// The vector type (of the vector space).
    typedef typename EuclideanSpaceType::VectorType VectorType;
    /// The scalar type (of the scalar field).
    typedef typename EuclideanSpaceType::ScalarType ScalarType;
    /// The point type (of the Euclidean space).
    typedef typename EuclideanSpaceType::PointType PointType;
    /// @brief @a MyType is the type of this template/template specialization.
    typedef Ray<EuclideanSpaceType> MyType;

public:
    /**
     * @brief Construct this ray with the specified origin point \f$O\f$ and axis vector \f$\vec{a}\f$.
     * @param origin the origin point \f$O\f$
     * @param direction the direction vector \f$\vec{d}\f$
     * @throw Id::RuntimeErrorException if the direction vector \f$\vec{d}\f$ is the zero vector 
     */
    Ray3(const PointType& origin, const VectorType& direction)
        : origin(origin), direction(directio) {
        direction = direction.normalize_new();
    }

    /** 
     * @brief Construct this ray with the values of another ray.
     * @param other the other ray
     */
    Ray(const Ray& other)
        : origin(other.origin), axis(other.axis) {
        /* Intentionally empty. */
    }

    /**
     * @brief Assign this ray with the values of another ray.
     * @param other the other ray
     * @return this ray
     */
    Ray operator=(const Ray& other) {
        origin = other.origin;
        axi = other.axis;
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
    /** @copydoc Ego::Math::Translatable */
    void translate(const VectorType& t) override {
        origin += t;
    }

}; // struct Ray

} // namespace Math
} // namespace Ego
